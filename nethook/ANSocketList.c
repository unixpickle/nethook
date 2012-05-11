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
    void * next;
} ANSocketList;

static ANSocketList * ANSocketListCreate(OSMallocTag tag) {
    ANSocketList * list = (ANSocketList *)OSMalloc(sizeof(ANSocketList), tag);
    if (!list) return NULL;
    bzero(list, sizeof(ANSocketList));
    return list;
}

static ANSocketList * ANSocketListAdd(OSMallocTag tag, ANSocketList * first, socket_t socket) {
    ANSocketList * newList = ANSocketListCreate(tag);
    newList->socket = socket;
    
    ANSocketList * last = first;
    while (last->next) last = last->next;
    
    last->next = newList;    
    return newList;
}

static void ANSocketListDelete(OSMallocTag tag, ANSocketList * first, ANSocketList * obj) {
    if (!obj) return;
    ANSocketList * list = first;
    while (list != NULL) {
        if (list->next == obj) {
            list->next = obj->next;
            break;
        }
        list = list->next;
    }
    OSFree(obj, sizeof(ANSocketList), tag);
}

static void ANSocketListFree(OSMallocTag tag, ANSocketList * first) {
    ANSocketList * node = first;
    while (node) {
        ANSocketList * next = (ANSocketList *)node->next;
        OSFree(node, sizeof(ANSocketList), tag);
        node = next;
    }
}
