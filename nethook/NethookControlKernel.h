//
//  NethookControlKernel.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_NethookControlKernel_h
#define nethook_NethookControlKernel_h

#include <sys/kpi_socket.h>
#include <libkern/OSMalloc.h>
#include <sys/kpi_mbuf.h>
#include <sys/mbuf.h>
#include <sys/kern_control.h>
#include <mach/mach_types.h>
#include <sys/param.h>


#include "NethookControl.h"

__private_extern__ ANPacketInfo * ANPacketInfoCreate(OSMallocTag tag, mbuf_t * buf, ANPacketType type, ANPacketProtocol protocol);
__private_extern__ void ANPacketInfoFree(OSMallocTag tag, ANPacketInfo * info);

#endif
