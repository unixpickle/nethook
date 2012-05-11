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
#include "ANSocketList.c"
#include "socketfilter.h"

static ANSocketList * sockList = NULL;
static OSMallocTag mallocTag = NULL;

static lck_grp_t * mutexGroup = NULL;
static lck_mtx_t * sockListMutex = NULL;

static Boolean shouldStop = FALSE;

#define debugf printf

kern_return_t nethook_start(kmod_info_t * ki, void * d);
kern_return_t nethook_stop(kmod_info_t * ki, void * d);

kern_return_t nethook_start(kmod_info_t * ki, void * d) {
    mallocTag = OSMalloc_Tagalloc(kBundleID, OSMT_DEFAULT);
    
    // create mutexes
    debugf("Initializing locks...\n");
    mutexGroup = lck_grp_alloc_init(kBundleID, LCK_GRP_ATTR_NULL);
    if (!mutexGroup) return KERN_FAILURE;
    sockListMutex = lck_mtx_alloc_init(mutexGroup, LCK_ATTR_NULL);
    if (!sockListMutex) return KERN_FAILURE;
    
    // create socket list
    sockList = ANSocketListCreate(mallocTag);
    if (!sockList) return KERN_FAILURE;
    
    // register as a network filter
    errno_t num = sflt_register(&NethookFilterInfo, PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (num != 0) {
        return KERN_FAILURE;
    }
    
    return KERN_SUCCESS;
}

kern_return_t nethook_stop(kmod_info_t * ki, void * d) {
    debugf("Got stop call\n");
    if (sockList) {
        if (sockList->next) {
            debugf("Too busy to stop\n");
            shouldStop = TRUE;
            return EBUSY;
        }
    }
    
    // unregister as a network filter
    sflt_unregister(kNethookHandlerMain);
    debugf("SF unregistered\n");
    
    // free existing stuff
    lck_mtx_lock(sockListMutex);
    if (sockList) {
        ANSocketListFree(mallocTag, sockList);
        sockList = NULL;
    }
    lck_mtx_unlock(sockListMutex);
    
    // free mutexes
    lck_mtx_free(sockListMutex, mutexGroup);
    lck_grp_free(mutexGroup);
    sockListMutex = NULL;
    mutexGroup = NULL;
        
    OSMalloc_Tagfree(mallocTag);
    mallocTag = NULL;
    return KERN_SUCCESS;
}

#pragma mark - Socket Filter General Callbacks -

static void nh_handle_unregister (sflt_handle handle) {
    //debugf("filter unregistered\n");
}

static errno_t nh_handle_attach (void ** cookie, socket_t so) {
    if (shouldStop) {
        *cookie = NULL;
        return ENXIO;
    }
    debugf("Attaching to a socket.\n");
    lck_mtx_lock(sockListMutex);
    ANSocketList * node = ANSocketListAdd(mallocTag, sockList, so);
    lck_mtx_unlock(sockListMutex);
    *cookie = node;
    return 0;
}

static void nh_handle_detach (void * cookie, socket_t so) {
    debugf("Detaching from socket.\n");
    lck_mtx_lock(sockListMutex);
    ANSocketList * node = (ANSocketList *)cookie;
    ANSocketListDelete(mallocTag, sockList, node);
    lck_mtx_unlock(sockListMutex);
}

static errno_t nh_handle_bind (void * cookie, socket_t so, const struct sockaddr * to) {
    lck_mtx_lock(sockListMutex);
    ANSocketList * node = (ANSocketList *)cookie;
    memcpy(node->addr, to, MIN(sizeof(struct sockaddr_in), 64));
    lck_mtx_unlock(sockListMutex);
    return 0;
}
