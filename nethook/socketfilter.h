//
//  socketfilter.h
//  nethook
//
//  Created by Alex Nichol on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_socketfilter_h
#define nethook_socketfilter_h

static void nh_handle_unregister (sflt_handle handle);
static errno_t nh_handle_attach (void ** cookie, socket_t so);
static void nh_handle_detach (void * cookie, socket_t so);
static errno_t nh_handle_bind (void * cookie, socket_t so, const struct sockaddr * to);

static struct sflt_filter NethookFilterInfo = {
    kNethookHandlerMain,
    SFLT_GLOBAL,
    kBundleID,
    nh_handle_unregister,
    nh_handle_attach,
    nh_handle_detach,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    nh_handle_bind,
    NULL,
    NULL,
    NULL,
    NULL
};

#endif
