//
//  nethook.c
//  nethook
//
//  Created by Alex Nichol on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <sys/socket.h>
#include <sys/kpi_mbuf.h>
#include <sys/kpi_socketfilter.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <kern/locks.h>
#include <kern/assert.h>
#include <kern/debug.h>

#include <libkern/OSMalloc.h>
#include <libkern/OSAtomic.h>
#include <sys/kern_control.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <stdarg.h>
#include <mach/mach_types.h>

#include "defines.h"
#include "control.h"

static OSMallocTag mallocTag = NULL;

static lck_grp_t * mutexGroup = NULL;
static lck_mtx_t * controlMutex = NULL;

static boolean_t controlRegistered = FALSE;
static ANControlList * controlList = NULL;
static kern_ctl_ref controlRef;

static void debugf(const char * str, ...);

kern_return_t nethook_start(kmod_info_t * ki, void * d);
kern_return_t nethook_stop(kmod_info_t * ki, void * d);

kern_return_t nethook_start(kmod_info_t * ki, void * d) {
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
    
    // register control
    
    errno_t error = ctl_register(&NethookControlReg, &controlRef);
    if (error != 0) {
        debugf("Fatal: failed to register control: %lu", error);
        return KERN_FAILURE;
    } else {
        controlRegistered = TRUE;
    }
    
    return KERN_SUCCESS;
}

kern_return_t nethook_stop(kmod_info_t * ki, void * d) {
    if (controlRegistered) {
        if (ctl_deregister(controlRef) != 0) {
            debugf("Error: cannot unload: clients are currently connected from user-space!");
            return EBUSY;
        } else {
            controlRegistered = FALSE;
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
    ANControlListEntry * entry = (ANControlListEntry *)controlList;
    lck_mtx_lock(controlMutex);
    ANControlListRemove(controlList, entry);
    lck_mtx_unlock(controlMutex);
    return 0;
}

static errno_t nh_control_handle_getopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t * len) {
    return 0;
}

static errno_t nh_control_handle_send(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, mbuf_t m, int flags) {
    debugf("Got data from control!");
    return 0;
}

static errno_t nh_control_handle_setopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t len) {
    return 0;
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
