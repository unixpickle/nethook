//
//  iplog.c
//  nethook
//
//  Created by Alex Nichol on 5/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "iplog.h"

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
    } else if (ipHeader.ip_p == 1) {
        char source[32], dest[32];
        strncpy(source, inet_ntoa(ipHeader.ip_src), 32);
        strncpy(dest, inet_ntoa(ipHeader.ip_dst), 32);
        printf("ICMP %s to %s\n", source, dest);
    } else {
        char source[32], dest[32];
        strncpy(source, inet_ntoa(ipHeader.ip_src), 32);
        strncpy(dest, inet_ntoa(ipHeader.ip_dst), 32);
        printf("IP %s to %s with protocol %d\n", source, dest, ipHeader.ip_p);
    }
}
