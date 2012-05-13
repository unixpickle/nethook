//
//  iplog.h
//  nethook
//
//  Created by Alex Nichol on 5/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef nethook_iplog_h
#define nethook_iplog_h

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

void handleTCPPacket (struct ip ipHeader, struct tcphdr tcpHeader, char * data, int length);
void handleUDPPacket (struct ip ipHeader, struct udphdr udpHeader, char * data, int length);
void handleIPPacket (struct ip ipHeader, void * data, int length);


#endif
