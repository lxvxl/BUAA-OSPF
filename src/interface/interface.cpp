#include "../../include/packet/packet_manage.h"
#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"

Neighbor* Interface::get_neighbor(uint32_t router_id) {
    for (auto n : neighbors) {
        if (n->router_id == router_id) {
            return n;
        }
    }
    return NULL;
}

void Interface::generate_hello_packet(OSPFHello *hello_packet) {
    // 填充 OSPFHeader
    hello_packet->header.version            = 2;  // OSPF 版本号
    hello_packet->header.type               = OSPFPacketType::HELLO;     // 报文类型 1 为 hello
    hello_packet->header.router_id          = this->router_id;  // 路由器 ID
    hello_packet->header.area_id            = this->area_id; 
    hello_packet->header.checksum           = 0; // 校验和，后面再计算
    hello_packet->header.authType           = 0; // 认证类型，假定为 0
    hello_packet->header.authentication[0]  = 0;
    hello_packet->header.authentication[1]  = 0;
    
    // 填充 OSPFHello 特定字段
    hello_packet->network_mask              = this->network_mask;
    hello_packet->hello_interval            = htons(this->hello_interval); // 转换为网络字节序
    hello_packet->options                   = 0; // 假定选项字段为 0
    hello_packet->rtr_priority              = this->rtr_priority;
    hello_packet->dead_interval             = htonl(this->dead_interval); // 转换为网络字节序
    hello_packet->designated_router         = this->designated_route;
    hello_packet->backup_designated_router  = this->backup_designated_router;

    // 填充邻居路由器列表
    int neighbor_count                      = this->neighbors.size();
    hello_packet->header.packet_length      = htons(sizeof(OSPFHello) + neighbor_count * sizeof(uint32_t) - sizeof(struct iphdr));

    for (int i = 0; i < neighbor_count; ++i) {
        hello_packet->neighbors[i] = this->neighbors[i]->router_id;
    }

    // 计算校验和（假定为简单的累加和）
    uint16_t *hello_packet_ptr = (uint16_t *)((char*)hello_packet + sizeof(iphdr));
    uint32_t sum = 0;
    for (int i = 0; i < 22 + neighbor_count * 2; ++i) {
        sum += ntohs(hello_packet_ptr[i]);
    }
    hello_packet->header.checksum = htons(~((sum & 0xFFFF) + (sum >> 16)));
}

