//
//  nethook.c
//  nethook
//
//  Created by Alex Nichol on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <mach/mach_types.h>

kern_return_t nethook_start(kmod_info_t * ki, void *d);
kern_return_t nethook_stop(kmod_info_t *ki, void *d);

kern_return_t nethook_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t nethook_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
