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
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#import "NethookControlUser.h"

int openControlSocket (NSString * bundleID);
void handleTCPPacket (struct ip ipHeader, struct tcphdr tcpHeader, char * data, int length);
void handleUDPPacket (struct ip ipHeader, struct udphdr udpHeader, char * data, int length);
void handleIPPacket (struct ip ipHeader, void * data, int length);

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        int fd = openControlSocket(@"com.aqnichol.nethook");
        if (fd < 0) {
            fprintf(stderr, "Failed to connect to nethook!\n");
            return -1;
        }
        printf("Connection success!\n");
        
        while (true) {
            ANPacketInfo * info = ANPacketInfoRead(fd);
            
            if (info->length - 8 >= sizeof(struct ip) && info->protocol == ANPacketProtocolIPv4) {
                struct ip header;
                memcpy(&header, info->data, sizeof(header));
                handleIPPacket(header, &info->data[header.ip_hl * 4], info->length - 8 - (header.ip_hl * 4));
            }
            
            ANPacketInfoFree(info);
        }
        
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

void handleTCPPacket (struct ip ipHeader, struct tcphdr tcpHeader, char * data, int length) {
    char source[32], dest[32];
    strncpy(source, inet_ntoa(ipHeader.ip_src), 32);
    strncpy(dest, inet_ntoa(ipHeader.ip_dst), 32);
    printf("TCP %s:%d to %s:%d\n", source, tcpHeader.th_sport, dest, tcpHeader.th_dport);
}

void handleUDPPacket (struct ip ipHeader, struct udphdr udpHeader, char * data, int length) {
    char source[32], dest[32];
    strncpy(source, inet_ntoa(ipHeader.ip_src), 32);
    strncpy(dest, inet_ntoa(ipHeader.ip_dst), 32);
    printf("UDP %s:%d to %s:%d\n", source, udpHeader.uh_sport, dest, udpHeader.uh_dport);
}

void handleIPPacket (struct ip ipHeader, void * data, int length) {
    if (ipHeader.ip_p == 6 && length >= sizeof(struct tcphdr)) {
        struct tcphdr tcpHeader;
        memcpy(&tcpHeader, data, sizeof(tcpHeader));
        int headerSize = tcpHeader.th_off * 4;
        handleTCPPacket(ipHeader, tcpHeader, &((char *)data)[headerSize], headerSize);
    } else if (ipHeader.ip_p == 17 && length >= sizeof(struct udphdr)) {
        struct udphdr udpHeader;
        memcpy(&udpHeader, data, sizeof(udpHeader));
        int headerSize = 8;
        handleUDPPacket(ipHeader, udpHeader, &((char *)data)[headerSize], headerSize);
    } else {
        char source[32], dest[32];
        strncpy(source, inet_ntoa(ipHeader.ip_src), 32);
        strncpy(dest, inet_ntoa(ipHeader.ip_dst), 32);
        printf("IP %s to %s with protocol %d\n", source, dest, ipHeader.ip_p);
    }
}
