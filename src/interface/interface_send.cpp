#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include <functional>


void Interface::generate_hello_packet() {
    // 填充 OSPFHeader
    OSPFHello *hello_packet                 = (struct OSPFHello*)(this->send_buffer);
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
    hello_packet->options                   = 2; // 假定选项字段为 2
    hello_packet->rtr_priority              = this->rtr_priority;
    hello_packet->dead_interval             = htonl(this->dead_interval); // 转换为网络字节序
    hello_packet->designated_router         = this->dr;
    hello_packet->backup_designated_router  = this->bdr;

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

void Interface::send_thread_runner() {
    printf("initing hello thread\n");
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, 89)) < 0) {
        perror("[Thread]SendHelloPacket: socket_fd init");
    }
    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, this->name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]SendHelloPacket: setsockopt");
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->hello_timer--;
        this->wait_timer--;
        if (this->hello_timer == 0) {
            this->hello_timer = 10;
            this->send_hello_packet(socket_fd);
        }
        if (this->wait_timer == 0 && this->state == InterfaceState::WAITING) {
            this->event_wait_timer();
        }
        for (auto neighbor : this->neighbors) {
            neighbor->inactivity_timer--;
            if (neighbor->inactivity_timer == 0) {
                neighbor->event_kill_nbr();
            }

        }
    }
}

void Interface::send_hello_packet(int socket_fd) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = inet_addr("224.0.0.5");
    this->generate_hello_packet();
    if (sendto(socket_fd, 
               this->send_buffer + 20, 
               44 + this->neighbors.size() * 4, 
               0, 
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]SendHelloPacket: sendto");
    } 
    printf("[hello thread] send a packet");
}
