#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"

void show_ipv4_header(struct iphdr *header) {
    printf("Version: %d\n", header->version);
    printf("Header Length: %d (20 bytes)\n", header->ihl * 4);
    printf("Type of Service: 0x%02x\n", header->tos);
    printf("Total Length: %d\n", ntohs(header->tot_len));
    printf("Identification: 0x%04x (%d)\n", ntohs(header->id), ntohs(header->id));
    printf("Fragment Offset: 0x%04x (%d)\n", ntohs(header->frag_off), ntohs(header->frag_off));
    printf("Time to Live (TTL): %d\n", header->ttl);
    printf("Protocol: %d\n", header->protocol);
    printf("Header Checksum: 0x%04x\n", ntohs(header->check));
    struct in_addr src_ip = {ntohl(header->saddr)};
    struct in_addr dst_ip = {ntohl(header->daddr)};
    printf("Source Address: %s\n", inet_ntoa(src_ip));
    printf("Destination Address: %s\n", inet_ntoa(dst_ip));
}
