//
//  nethook_list.c
//  nethook
//
//  Created by Alex Nichol on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <sys/kpi_socket.h>

typedef struct {
    socket_t socket;
    char addr[64];
    void * next, * last;
} ANSocketList;

static ANSocketList * ANSocketListCreate(OSMallocTag tag) {
    ANSocketList * list = (ANSocketList *)OSMalloc(sizeof(ANSocketList), tag);
    if (!list) return NULL;
    bzero(list, sizeof(ANSocketList));
    return list;
}

static void ANSocketListAdd(OSMallocTag tag, ANSocketList * first, socket_t socket) {
    ANSocketList * newList = ANSocketListCreate(tag);
    newList->socket = socket;
    ANSocketList * last = first;
    while (last->next) last = last->next;
    last->next = newList;
}

static void ANSocketListDelete(OSMallocTag tag, ANSocketList * obj) {
    if (obj->last) {
        ((ANSocketList *)obj->last)->next = obj->next;
    }
    if (obj->next) {
        ((ANSocketList *)obj->next)->last = obj->last;
    }
    OSFree(obj, sizeof(ANSocketList), tag);
}

static void ANSocketListFree(OSMallocTag tag, ANSocketList * first) {
    while (first->next) {
        ANSocketListDelete(tag, (ANSocketList *)first->next);
    }
    ANSocketListDelete(tag, first);
}
