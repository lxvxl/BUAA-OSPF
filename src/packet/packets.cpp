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
    printf("Packet lenth: %d\n", ntohs(packet_length));
    printf("Router Id: %s\n", ip_to_string(router_id));
    printf("Area Id: %s\n", ip_to_string(area_id));
}

void OSPFHello::show() {
    header.show();
    printf("Network Mask: %s\n", ip_to_string(network_mask));
    printf("Hello Interval: %d\n", ntohs(hello_interval)); // 转换为主机字节序
    printf("Router Priority: %d\n", rtr_priority);
    printf("Dead Interval: %d\n", ntohl(dead_interval)); // 转换为主机字节序
    printf("Designated Router: %s\n", ip_to_string(designated_router));
    printf("Backup Designated Router: %s\n", ip_to_string(backup_designated_router));
    printf("Neighbors: ");
    for (int i = 0; i < get_neighbor_num(); i++) {
        printf("%s ", ip_to_string(neighbors[i]));
    }
    printf("\n");
}

int OSPFHello::get_neighbor_num() {
    return (ntohs(header.packet_length) - 44) / 4;
}

bool OSPFHello::has_neighbor(uint32_t router_id) {
    for (int i = 0; i < get_neighbor_num(); i++) {
        if (neighbors[i] == router_id) {
            return true;
        }
    }
    return false;
}  

void OSPFDD::show() {
    header.show();
    printf("Interface MTU: %d\n", ntohs(interface_mtu)); // 转换为主机字节序
    printf("Options: %d\n", options);
    printf("MS: %d\n", b_MS);
    printf("M: %d\n", b_M);
    printf("I: %d\n", b_I);
    printf("Sequence Number: %d\n", ntohl(dd_sequence_number)); // 转换为主机字节序
}

void OSPFLSR::show() {
    header.show();
    printf("LS Type: %d\n", ntohl(ls_type)); // 转换为主机字节序
    printf("Link State ID: %d\n", ntohl(link_state_id)); // 转换为主机字节序
    printf("Advertising Router: %d\n", ntohl(advertising_router)); // 转换为主机字节序
}

void OSPFLSU::show() {
    header.show();
    printf("Number of LSAs: %d\n", ntohl(num_lsas)); // 转换为主机字节序
}

void OSPFLSAck::show() {
    header.show();
    printf("LS Type: %d\n", ntohl(ls_type)); // 转换为主机字节序
    printf("Link State ID: %d\n", ntohl(link_state_id)); // 转换为主机字节序
    printf("Advertising Router: %d\n", ntohl(advertising_router)); // 转换为主机字节序
    printf("LS Sequence Number: %d\n", ntohl(ls_sequence_number)); // 转换为主机字节序
    printf("LS Age: %d\n", ntohs(ls_age)); // 转换为主机字节序
    printf("Checksum: %d\n", ntohs(checksum)); // 转换为主机字节序
}
