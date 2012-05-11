//
//  ANControlList.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <sys/systm.h>
#include <libkern/OSMalloc.h>
#include <sys/kern_control.h>
#include <mach/mach_types.h>

#ifndef nethook_ANControlList_h
#define nethook_ANControlList_h

typedef struct {
    uint32_t unit;
    kern_ctl_ref ctlref;
    boolean_t connected;
    void * next;
} ANControlListEntry;

typedef struct {
    ANControlListEntry * first;
    int count;
    OSMallocTag tag;
} ANControlList;

__private_extern__ ANControlList * ANControlListCreate(OSMallocTag tag);
__private_extern__ void ANControlListFree(ANControlList * list);
__private_extern__ ANControlListEntry * ANControlListAdd(ANControlList * list, uint32_t unit, kern_ctl_ref ctlref);
__private_extern__ void ANControlListRemove(ANControlList * list, ANControlListEntry * entry);
__private_extern__ ANControlListEntry * ANControlListLookupByUnit(ANControlList * list, uint32_t unit);

__private_extern__ ANControlListEntry * ANControlListEntryCreate(OSMallocTag tag, uint32_t unit, kern_ctl_ref ctlref);
__private_extern__ void ANControlListEntryFree(OSMallocTag tag, ANControlListEntry * entry);

#endif
