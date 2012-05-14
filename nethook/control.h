//
//  control.h
//  nethook
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_control_h
#define nethook_control_h

#include <netinet/kpi_ipfilter.h>

#include "ANControlList.h"
#include "defines.h"

static errno_t nh_control_handle_connect(kern_ctl_ref kctlref, struct sockaddr_ctl * sac, void ** unitinfo);
static errno_t nh_control_handle_disconnect(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo);
static errno_t nh_control_handle_getopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t * len);
static errno_t nh_control_handle_send(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, mbuf_t m, int flags);
static errno_t nh_control_handle_setopt(kern_ctl_ref kctlref, u_int32_t unit, void * unitinfo, int opt, void * data, size_t len);
static errno_t nh_control_forward_output(mbuf_t packet, ipfilter_t filter);

static struct kern_ctl_reg NethookControlReg = {
    kBundleID,
    0,
    0,
    CTL_FLAG_PRIVILEGED | CTL_FLAG_REG_SOCK_STREAM, // you can remove CTL_FLAG_PRIVILEGED to give permission to *anybody*
    0,
    0,
    nh_control_handle_connect,
    nh_control_handle_disconnect,
    nh_control_handle_send,
    nh_control_handle_setopt,
    nh_control_handle_getopt
};

#endif
