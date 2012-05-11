//
//  main.m
//  nethookclient
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>

int openControlSocket (NSString * bundleID);

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        int fd = openControlSocket(@"com.aqnichol.nethook");
        if (fd < 0) {
            fprintf(stderr, "Failed to connect to nethook!\n");
            return -1;
        }
        printf("Connection success!\n");
        sleep(5);
        close(fd);
    }
    return 0;
}

int openControlSocket (NSString * bundleID) {
    const char * addrStr = [bundleID UTF8String];
    
    struct sockaddr_ctl addr;
    struct ctl_info info;
    bzero(&addr, sizeof(addr));
    bzero(&info, sizeof(info));
    
    addr.sc_len = sizeof(struct sockaddr_ctl);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    
    strncpy(info.ctl_name, addrStr, sizeof(info.ctl_name));
    
    int fd = socket(PF_SYSTEM, SOCK_STREAM, SYSPROTO_CONTROL);
    if (fd < 0) return fd;
    
    if (ioctl(fd, CTLIOCGINFO, &info)) {
        return ENOENT;
    }
    
    addr.sc_id = info.ctl_id;
    addr.sc_unit = 0;
    
    int result;
    if ((result = connect(fd, (struct sockaddr *)&addr, sizeof(addr)))) {
        return result;
    }
    
    return fd;
}
