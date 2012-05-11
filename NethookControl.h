//
//  NethookControl.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_NethookControl_h
#define nethook_NethookControl_h

typedef enum {
    ANPacketTypeInbound = 1,
    ANPacketTypeOutbound = 2
} ANPacketType;

typedef enum {
    ANPacketProtocolIPv4 = 1
} ANPacketProtocol;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t protocol;
    uint32_t length;
    char data[1];
} ANPacketInfo;

#endif
