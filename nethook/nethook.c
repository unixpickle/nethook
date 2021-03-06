//
//  nethook.c
//  nethook
//
//  Created by Alex Nichol on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <sys/socket.h>
#include <sys/kpi_mbuf.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <kern/locks.h>
#include <kern/assert.h>
#include <kern/debug.h>

#include <libkern/OSMalloc.h>
#include <libkern/OSAtomic.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <stdarg.h>
#include <net/if_var.h>

#include "defines.h"
#include "control.h"
#include "ipfilter.h"

static OSMallocTag mallocTag = NULL;

static lck_grp_t * mutexGroup = NULL;
static lck_mtx_t * controlMutex = NULL;

static boolean_t controlRegistered = FALSE;
static ANControlList * controlList = NULL;
static kern_ctl_ref controlRef;

static boolean_t filterRegistered = FALSE;
static ipfilter_t filter = NULL;

static mbuf_tag_id_t filterTagID;
static boolean_t filterHasTag = FALSE;
#define ANFilterTagType 1


static void debugf(const char * str, ...);

kern_return_t nethook_start(kmod_info_t * ki, void * d);
kern_return_t nethook_stop(kmod_info_t * ki, void * d);
static errno_t packet_to_mbuf(ANPacketInfo * packet, mbuf_t * outBuffer);

kern_return_t nethook_start(kmod_info_t * ki, void * d) {
    errno_t error;
    
    mallocTag = OSMalloc_Tagalloc(kBundleID, OSMT_DEFAULT);
    
    // create mutexes
    debugf("Initializing locks...");
    mutexGroup = lck_grp_alloc_init(kBundleID, LCK_GRP_ATTR_NULL);
    if (!mutexGroup) return KERN_FAILURE;
    controlMutex = lck_mtx_alloc_init(mutexGroup, LCK_ATTR_NULL);
    if (!controlMutex) return KERN_FAILURE;
    debugf("Locks initialized");
    
    // create control list
    controlList = ANControlListCreate(mallocTag);
    if (!controlList) {
        debugf("Fatal: failed to create control list!");
        return KERN_FAILURE;
    }
    
    // create our mbuf tag
    error = mbuf_tag_id_find(kBundleID, &filterTagID);
    if (error != 0) {
        debugf("Warning: failed to create mbuf tag");
    } else {
        filterHasTag = TRUE;
    }
    
    // create IP filter
    debugf("Registering the IPv4 filter...");
    error = ipf_addv4(&NethookIPFilterDescription, &filter);
    if (error != 0) {
        debugf("Fatal: failed to register IPv4 filter!");
        return KERN_FAILURE;
    } else {
        filterRegistered = TRUE;
    }
    debugf("IPv4 filter registered successfully");
    
    // register control
    debugf("Registering the control interface...");
    error = ctl_register(&NethookControlReg, &controlRef);
    if (error != 0) {
        debugf("Fatal: failed to register control: %lu", error);
        return KERN_FAILURE;
    } else {
        controlRegistered = TRUE;
    }
    debugf("Control interface registered successfully");
    
    return KERN_SUCCESS;
}

kern_return_t nethook_stop(kmod_info_t * ki, void * d) {
    // deregister control
    if (controlRegistered) {
        if (ctl_deregister(controlRef) != 0) {
            debugf("Error: cannot unload: clients are currently connected from user-space!");
            return EBUSY;
        } else {
            controlRegistered = FALSE;
        }
    }
    
    // remove filter
    if (filterRegistered) {
        if (ipf_remove(filter) != 0) {
            debugf("Error: cannot unload IP filter!");
            return EBUSY;
        } else {
            filterRegistered = FALSE;
        }
    }
    
    // free control list
    ANControlListFree(controlList);
    
    // free mutexes
    lck_mtx_free(controlMutex, mutexGroup);
    lck_grp_free(mutexGroup);
    controlMutex = NULL;
    mutexGroup = NULL;
        
    OSMalloc_Tagfree(mallocTag);
    mallocTag = NULL;
    return KERN_SUCCESS;
}

#pragma mark - Control -

static errno_t nh_control_handle_connect(kern_ctl_ref kctlref, struct sockaddr_ctl * sac, void ** unitinfo) {
    debugf("connected by external control");
    lck_mtx_lock(controlMutex);
    ANControlListEntry * entry = ANControlListAdd(controlList, sac->sc_unit, kctlref);
    lck_mtx_unlock(controlMutex);
    *unitinfo = entry;
    return 0;
}

static errno_t nh_control_handle_disconnect(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo) {
    debugf("disconnected by external control");
    ANControlListEntry * entry = (ANControlListEntry *)unitinfo;
    lck_mtx_lock(controlMutex);
    ANControlListRemove(controlList, entry);
    lck_mtx_unlock(controlMutex);
    return 0;
}

static errno_t nh_control_handle_getopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t * len) {
    return 0;
}

static errno_t nh_control_handle_send(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, mbuf_t m, int flags) {
    ANControlListEntry * entry = (ANControlListEntry *)unitinfo;
    lck_mtx_lock(controlMutex);
    debugf("Appending packet...");
    if (ANControlListEntryAppend(mallocTag, entry, m)) {
        debugf("Failed to append list entry...");
    }
    ANPacketInfo * packet = ANControlListEntryGetPacket(mallocTag, entry);
    lck_mtx_unlock(controlMutex);
    if (packet) {
        debugf("Got packet from user-space, injecting...");
        errno_t injectError = 0;
        
        if (packet->type == ANPacketTypeInbound) {
            injectError = nh_control_forward_input(packet, filter);
        } else {
            injectError = nh_control_forward_output(packet, filter);
        }
        
        ANPacketInfoFree(mallocTag, packet);
        if (injectError) {
            debugf("Inject error: %d", injectError);
        } else {
            debugf("Injection successful!");
        }
        return 0;
    }
    return 0;
}

static errno_t nh_control_handle_setopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t len) {
    return 0;
}

static errno_t nh_control_forward_input(ANPacketInfo * packet, ipfilter_t theFilter) {
    mbuf_t headerPacket;
    errno_t error = packet_to_mbuf(packet, &headerPacket);
    if (error) {
        debugf("Failed to create mbuf with error: %d", error);
        return error;
    }
    
    error = ipf_inject_input(headerPacket, theFilter);
    if (error) {
        mbuf_freem(headerPacket);
    }
    return error;
}

static errno_t nh_control_forward_output(ANPacketInfo * packet, ipfilter_t theFilter) {
    mbuf_t headerPacket;
    errno_t error = packet_to_mbuf(packet, &headerPacket);
    if (error) {
        debugf("Failed to create mbuf with error: %d", error);
        return error;
    }

    ipf_pktopts_t options = (ipf_pktopts_t)OSMalloc(sizeof(struct ipf_pktopts), mallocTag);
    bzero(options, sizeof(struct ipf_pktopts));
    options->ippo_flags = 2;
    error = ipf_inject_output(headerPacket, theFilter, options);
    
    // TODO: figure out if (and where) we need to free `options'
    if (error != 0) {
        OSFree(options, sizeof(struct ipf_pktopts), mallocTag);
        mbuf_freem(headerPacket);
    }
    return error;
}

static errno_t packet_to_mbuf(ANPacketInfo * packet, mbuf_t * outBuffer) {
    mbuf_t headerPacket;
    if (mbuf_allocpacket(MBUF_WAITOK, packet->length - 8, NULL, &headerPacket)) {
        return ENOMEM;
    }

    mbuf_copyback(headerPacket, 0, packet->length - 8, packet->data, MBUF_WAITOK);
    mbuf_setflags(headerPacket, MBUF_PKTHDR/* | MBUF_LOOP*/); // you don't seem to need the second flag
    mbuf_settype(headerPacket, MBUF_TYPE_DATA);
    mbuf_pkthdr_setlen(headerPacket, packet->length - 8);

    *outBuffer = headerPacket;
    return 0;
}

#pragma mark - IP Filter -

static errno_t nh_tag_packet(mbuf_t * packet) {
    if (filterHasTag) {
        void * tagData = NULL;
        size_t tagLen = 0;
        if (mbuf_tag_find(*packet, filterTagID, ANFilterTagType, &tagLen, &tagData) == 0) {
            return EEXIST;
        }
        mbuf_tag_allocate(*packet, filterTagID, ANFilterTagType, 1, MBUF_DONTWAIT, &tagData);
    }
    return 0;
}

static errno_t nh_packet_broadcast(ANPacketInfo * info) {
    lck_mtx_lock(controlMutex);
    ANControlListEntry * entry = controlList->first;
    while (entry) {
        ctl_enqueuedata(entry->ctlref, entry->unit, info, info->length, 0);
        entry = entry->next;
    }
    lck_mtx_unlock(controlMutex);
    return 0;
}

static errno_t nh_ipfilter_handle_input(void * cookie, mbuf_t * data, int offset, u_int8_t protocol) {
    if (nh_tag_packet(data)) return 0;
    ANPacketInfo * info = ANPacketInfoCreate(mallocTag, data, ANPacketTypeInbound, ANPacketProtocolIPv4);
    if (!info) {
        debugf("Warning: failed to allocate packet info!");
        return 0;
    }
    nh_packet_broadcast(info);
    ANPacketInfoFree(mallocTag, info);
    return 0;
}

static errno_t nh_ipfilter_handle_output(void * cookie, mbuf_t * data, ipf_pktopts_t options) {
    if (nh_tag_packet(data)) return 0;
    ANPacketInfo * info = ANPacketInfoCreate(mallocTag, data, ANPacketTypeOutbound, ANPacketProtocolIPv4);
    if (!info) {
        debugf("Warning: failed to allocate packet info!");
        return 0;
    }
    nh_packet_broadcast(info);
    ANPacketInfoFree(mallocTag, info);
    return 0;
}

static void nh_ipfilter_handle_detach(void * cookie) {
}

#pragma mark - Miscellaneous -

static void debugf(const char * str, ...) {
    va_list list;
    va_start(list, str);
    char buff[128];
    vsnprintf(buff, 128, str, list);
    printf("["kBundleID"]: %s\n", buff);
    va_end(list);
}
