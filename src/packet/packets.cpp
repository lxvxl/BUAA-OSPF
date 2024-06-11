#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"

char* ip_to_string(uint32_t ip) {
    struct in_addr addr = {ip};
    return inet_ntoa(addr);
}

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
    printf("Source Address: %s\n", ip_to_string(header->saddr));
    printf("Destination Address: %s\n", ip_to_string(header->daddr));
}

void OSPFHeader::show() {
    printf("Packet lenth: %d\n", packet_length);
    printf("Router Id: %s\n", ip_to_string(router_id));
    printf("Area Id: %s\n", ip_to_string(area_id));
}

void OSPFHello::show() {
    header.show();
    printf("Network Mask: %s\n", ip_to_string(network_mask));
    printf("Hello Interval: %d\n", hello_interval);
    printf("Router Priority: %d\n", rtr_priority);
    printf("Dead Interval: %d\n", dead_interval);
    printf("Designated Router: %s\n", ip_to_string(designated_router));
    printf("Backup Designated Router: %s\n", ip_to_string(backup_designated_router));
}

void OSPFDD::show() {
    header.show();
    printf("Interface MTU: %d\n", interface_mtu);
    printf("Options: %d\n", options);
    printf("MS: %d\n", b_MS);
    printf("M: %d\n", b_M);
    printf("I: %d\n", b_I);
    printf("Sequence Number: %d\n", dd_sequence_number);
}

void OSPFLSR::show() {
    header.show();
    printf("LS Type: %d\n", ls_type);
    printf("Link State ID: %d\n", link_state_id);
    printf("Advertising Router: %d\n", advertising_router);
}

void OSPFLSU::show() {
    header.show();
    printf("Number of LSAs: %d\n", num_lsas);
}

void OSPFLSA::show() {
    header.show();
    printf("LS Type: %d\n", ls_type);
    printf("Link State ID: %d\n", link_state_id);
    printf("Advertising Router: %d\n", advertising_router);
    printf("LS Sequence Number: %d\n", ls_sequence_number);
    printf("LS Age: %d\n", ls_age);
    printf("Checksum: %d\n", checksum);
}
