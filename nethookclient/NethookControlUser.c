//
//  NethookControlUser.c
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "NethookControlUser.h"

ANPacketInfo * ANPacketInfoRead(int fd) {
    ANPacketInfo header;
    if (recv(fd, &header, 8, MSG_WAITALL) != 8) return NULL;
    
    ANPacketInfo * trueHeader = malloc(MAX(sizeof(header), header.length));
    memcpy(trueHeader, &header, 8);

    uint32_t dataLen = header.length - 8;
    if (dataLen != 0) {
        if (recv(fd, &trueHeader->data, dataLen, MSG_WAITALL) != dataLen) {
            free(trueHeader);
            return NULL;
        }
    }
    
    return trueHeader;
}

void ANPacketInfoFree(ANPacketInfo * packet) {
    free(packet);
}
