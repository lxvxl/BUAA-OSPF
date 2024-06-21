#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"


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

void OSPFHeader::fill(OSPFPacketType type, uint32_t area_id, uint16_t packet_length) {
    this->version            = 2;  // OSPF 版本号
    this->type               = type;     // 报文类型 1 为 hello
    this->packet_length      = htons(packet_length);
    this->router_id          = router::router_id;  // 路由器 ID
    this->area_id            = area_id; 
    this->checksum           = 0; // 校验和，后面再计算
    this->authType           = 0; // 认证类型，假定为 0
    this->authentication[0]  = 0;
    this->authentication[1]  = 0;
    
    uint16_t *packet_ptr = (uint16_t *)((char*)this);
    uint32_t sum = 0;
    for (int i = 0; i < packet_length / 2; ++i) {
        sum += ntohs(packet_ptr[i]);
    }
    this->checksum          = htons(~((sum & 0xFFFF) + (sum >> 16)));
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

void OSPFHello::fill(Interface *interface) {
    // 填充 OSPFHello 特定字段
    this->network_mask              = interface->network_mask;
    this->hello_interval            = htons(interface->hello_interval); // 转换为网络字节序
    this->options                   = router::config::options; // 假定选项字段为 2
    this->rtr_priority              = interface->rtr_priority;
    this->dead_interval             = htonl(interface->dead_interval); // 转换为网络字节序
    this->designated_router         = interface->dr;
    this->backup_designated_router  = interface->bdr;

    // 填充邻居路由器列表
    int neighbor_count              = interface->neighbors.size();
    for (int i = 0; i < neighbor_count; ++i) {
        this->neighbors[i] = interface->neighbors[i]->router_id;
    }
    ((OSPFHeader*)this)->fill(OSPFPacketType::HELLO, interface->area_id, sizeof(OSPFHello) + neighbor_count * 4);
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

int OSPFDD::get_lsa_num() {
    return (ntohs(this->header.packet_length) - sizeof(OSPFDD)) / sizeof(LSAHeader);
}

void OSPFDD::fill(Neighbor *neighbor) {
    this->interface_mtu      = htons(router::config::MTU);   // 接口MTU
    this->options            = router::config::options;
    this->b_MS               = neighbor->b_MS;
    this->b_I                = neighbor->b_I;
    this->b_other            = 0;
    this->dd_sequence_number = htonl(neighbor->dd_sequence_number); // DD序列号
    neighbor->dd_sequence_number++;

    Interface *interface = neighbor->interface;
    int header_num;
    if (this->b_I) {
        header_num = 0; 
    } else {
        header_num = neighbor->fill_lsa_headers(this->lsa_headers);
    } 
    this->b_M                = neighbor->dd_has_more_lsa();
    
    ((OSPFHeader*)this)->fill(OSPFPacketType::DD, interface->area_id, sizeof(OSPFDD) + header_num * sizeof(LSAHeader));
}


//void OSPFLSR::show() {
//    header.show();
//    printf("LS Type: %d\n", ntohl(ls_type)); // 转换为主机字节序
//    printf("Link State ID: %d\n", ntohl(link_state_id)); // 转换为主机字节序
//    printf("Advertising Router: %d\n", ntohl(advertising_router)); // 转换为主机字节序
//}
void OSPFLSR::fill(std::vector<LSAHeader*>& headers, Interface *interface) {
    size_t req_num = headers.size();
    for (int i = 0; i < req_num; i++) {
        this->reqs[i].ls_type            = htonl((uint32_t)headers[i]->ls_type);
        this->reqs[i].link_state_id      = headers[i]->link_state_id;
        this->reqs[i].advertising_router = headers[i]->advertising_router;
    }
    ((OSPFHeader*)this)->fill(OSPFPacketType::LSR, interface->area_id, sizeof(OSPFLSR) + req_num * sizeof(ReqItem));
}

int OSPFLSR::get_req_num() {
    return (ntohs(this->header.packet_length) - sizeof(OSPFLSR)) / sizeof(ReqItem);
}


void OSPFLSU::show() {
    header.show();
    printf("Number of LSAs: %d\n", ntohl(lsa_num)); // 转换为主机字节序
}

void OSPFLSU::fill(std::vector<LSAHeader*>& r_lsas, Interface *interface) {
    uint32_t lsa_num = r_lsas.size();
    uint8_t  *lsa_p = (uint8_t*)this + sizeof(OSPFLSU); 
    this->lsa_num = htonl(lsa_num);
    for (auto lsa : r_lsas) {
        memcpy(lsa_p, lsa, lsa->length);
        switch(lsa->ls_type) {
            case ROUTER:
                ((RouterLSA*)lsa_p)->hton();
                break;
            case NETWORK:
                ((NetworkLSA*)lsa_p)->hton();
                break;
        }
        lsa_p += lsa->length;
    }
    ((OSPFHeader*)this)->fill(OSPFPacketType::LSU, interface->area_id, lsa_p - (uint8_t*)this);
}

void OSPFLSAck::fill(std::vector<LSAHeader*>& v_lsas, Interface *interface) {
    uint32_t lsa_num = v_lsas.size();
    for (int i = 0; i < lsa_num; i++) {
        this->lsa_headers[i] = *v_lsas[i];
        this->lsa_headers[i].hton();
        std::cout<<this->lsa_headers[i].ls_checksum<<std::endl;
    }
    ((OSPFHeader*)this)->fill(OSPFPacketType::LSA, interface->area_id, sizeof(OSPFLSAck) + lsa_num * sizeof(LSAHeader));
}

int OSPFLSAck::get_lsa_num() {
    return (ntohs(this->header.packet_length) - sizeof(OSPFLSAck)) / sizeof(LSAHeader);
}