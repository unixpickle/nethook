//
//  ipfilter.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_ipfilter_h
#define nethook_ipfilter_h

#include "defines.h"
#include "NethookControlKernel.h"
#include <netinet/kpi_ipfilter.h>

static errno_t nh_tag_packet(mbuf_t * packet);
static errno_t nh_packet_broadcast(ANPacketInfo * info);

static errno_t nh_ipfilter_handle_input(void * cookie, mbuf_t * data, int offset, u_int8_t protocol);
static errno_t nh_ipfilter_handle_output(void * cookie, mbuf_t * data, ipf_pktopts_t options);
static void nh_ipfilter_handle_detach(void * cookie);

struct ipf_filter NethookIPFilterDescription = {
    NULL,
    kBundleID,
    nh_ipfilter_handle_input,
    nh_ipfilter_handle_output,
    nh_ipfilter_handle_detach
};

#endif
