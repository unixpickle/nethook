//
//  NethookControlKernel.c
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "NethookControlKernel.h"

__private_extern__ ANPacketInfo * ANPacketInfoCreate(OSMallocTag tag, mbuf_t * buf, ANPacketType type, ANPacketProtocol protocol) {
    size_t length = mbuf_len(*buf);
    uint32_t allocSize = (uint32_t)(sizeof(ANPacketInfo) + length) - 1;
    ANPacketInfo * info = (ANPacketInfo *)OSMalloc_noblock(allocSize, tag);
    if (!info) return NULL;
    
    info->length = allocSize;
    info->type = type;
    info->protocol = protocol;
    
    if (mbuf_copydata(*buf, 0, length, &info->data) != 0) {
        OSFree(info, allocSize, tag);
        return NULL;
    }
    
    return info;
}

__private_extern__ void ANPacketInfoFree(OSMallocTag tag, ANPacketInfo * info) {
    OSFree(info, info->length, tag);
}
