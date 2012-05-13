//
//  main.m
//  nethookclient
//
//  Created by Alex Nichol on 5/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include "NethookControlUser.h"
#include "iplog.h"

void connectionLog(int fd, int argc, const char * argv[]);
void icmpPing(int fd, int argc, const char * argv[]);
u_short in_cksum(u_short * addr, int len) ;

int openControlSocket (NSString * bundleID);

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        if (argc == 1) {
            fprintf(stderr, "Usage: %s <cmd> <args>\n", argv[0]);
            fprintf(stderr, "Commands:\n");
            fprintf(stderr, " icmp <host>\n");
            fprintf(stderr, " list\n\n");
            fflush(stderr);
            return 0;
        }
        
        int fd = openControlSocket(@"com.aqnichol.nethook");
        if (fd < 0) {
            fprintf(stderr, "Failed to connect to nethook!\n");
            return -1;
        }
        printf("Connection success!\n");
        
        if (strcmp(argv[1], "icmp") == 0) {
            icmpPing(fd, argc - 1, &argv[1]);
        } else if (strcmp(argv[1], "list") == 0) {
            connectionLog(fd, argc - 1, &argv[1]);
        } else {
            fprintf(stderr, "Unknown command: %s\n", argv[1]);
            fflush(stderr);
        }
        
        close(fd);
    }
    return 0;
}

void connectionLog(int fd, int argc, const char * argv[]) {
    while (true) {
        ANPacketInfo * info = ANPacketInfoRead(fd);
        if (!info) {
            fprintf(stderr, "Failed to read packet...\n");
            return;
        }
        
        if (info->length - 8 >= sizeof(struct ip) && info->protocol == ANPacketProtocolIPv4) {
            struct ip header;
            memcpy(&header, info->data, sizeof(header));
            handleIPPacket(header, &info->data[header.ip_hl * 4], info->length - 8 - (header.ip_hl * 4));
        }
        
        ANPacketInfoFree(info);
    }
}

void icmpPing(int fd, int argc, const char * argv[]) {
    if (argc != 2) {
        fprintf(stderr, "The `%s' command takes exactly 1 argument!\n", argv[0]);
        fflush(stderr);
        return;
    }
    
    struct in_addr dest;
    struct hostent * host = gethostbyname(argv[1]);
    if (!host) {
        fprintf(stderr, "Host not found\n");
        fflush(stderr);
        return;
    }
    memcpy(&dest, *host->h_addr_list, sizeof(dest));
    
    struct icmp pingHeader;
    bzero(&pingHeader, sizeof(struct icmp));
    pingHeader.icmp_type = 8; // echo request
    pingHeader.icmp_cksum = in_cksum((u_short *)&pingHeader, sizeof(struct icmp));
    struct ip ipHeader;
    bzero(&ipHeader, sizeof(ipHeader));
    ipHeader.ip_dst = dest;
    ipHeader.ip_p = 1;
    ipHeader.ip_ttl = 30;
    ipHeader.ip_v = 4;
    ipHeader.ip_hl = 5;
    ipHeader.ip_len = sizeof(ipHeader) + sizeof(pingHeader);
    ipHeader.ip_id = 0;
    ipHeader.ip_sum = in_cksum((u_short *)&ipHeader, sizeof(ipHeader));
    
    size_t size = sizeof(struct ip) + sizeof(struct icmp);
    ANPacketInfo * info = (ANPacketInfo *)malloc(8 + size);
    info->length = (uint32_t)(8 + size);
    info->type = ANPacketTypeOutbound;
    info->protocol = ANPacketProtocolIPv4;
    
    memcpy(info->data, &ipHeader, sizeof(ipHeader));
    memcpy(&info->data[sizeof(ipHeader)], &pingHeader, sizeof(pingHeader));
    send(fd, info, size + 8, MSG_WAITALL);
    
    free(info);
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

// Checksum code taken from http://www.angio.net/security/icmpquery.c
u_short in_cksum(u_short *addr, int len) {
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;
    
	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}
    
	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}
    
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

