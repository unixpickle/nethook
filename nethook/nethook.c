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

static ANSocketList * sockList = NULL;
static OSMallocTag mallocTag;

static lck_grp_t * mutexGroup = NULL;
static lck_mtx_t * sockListMutex = NULL;

kern_return_t nethook_start(kmod_info_t * ki, void * d);
kern_return_t nethook_stop(kmod_info_t * ki, void * d);

kern_return_t nethook_start(kmod_info_t * ki, void * d) {
    mallocTag = OSMalloc_Tagalloc(kBundleID, OSMT_DEFAULT);
    
    // create mutexes
    mutexGroup = lck_grp_alloc_init(kBundleID, LCK_GRP_ATTR_NULL);
    if (!mutexGroup) return KERN_FAILURE;
    sockListMutex = lck_mtx_alloc_init(mutexGroup, LCK_ATTR_NULL);
    if (!sockListMutex) return KERN_FAILURE;
    
    sockList = ANSocketListCreate(mallocTag);
    if (!sockList) return KERN_FAILURE;
    
    return KERN_SUCCESS;
}

kern_return_t nethook_stop(kmod_info_t * ki, void * d) {
    // free mutexes
    lck_mtx_free(sockListMutex, mutexGroup);
    lck_grp_free(mutexGroup);
    
    // free socket list
    ANSocketListFree(mallocTag, sockList);
    
    OSMalloc_Tagfree(mallocTag);
    mallocTag = NULL;
    return KERN_SUCCESS;
}
