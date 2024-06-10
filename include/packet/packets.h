#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/ip.h>


void show_ipv4_header(struct iphdr *header);

#endif // PACKET_MANAGE_H