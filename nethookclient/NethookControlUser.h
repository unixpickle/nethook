//
//  NethookControlUser.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_NethookControlUser_h
#define nethook_NethookControlUser_h

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include "NethookControl.h"

#ifndef MAX
#define MAX(x, y) (x < y ? y : x)
#endif

#ifndef MIN
#define MIN(x, y) (x > y ? y : x)
#endif

ANPacketInfo * ANPacketInfoRead(int fd);
void ANPacketInfoFree(ANPacketInfo * packet);

#endif
