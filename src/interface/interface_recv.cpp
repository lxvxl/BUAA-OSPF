#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/neighbor/neighbor.h"
#include <functional>

void recv_thread_runner(Interface *interface);
void handle_recv_hello(OSPFHello *hello_packet, Interface *interface);
void handle_recv_dd(OSPFDD *dd_packet);
void handle_recv_lsr(OSPFLSR *lsr_packet);
void handle_recv_lsu(OSPFLSU *lsu_packet);
void handle_recv_lsa(OSPFLSAck *lsa_packet);


void Interface::recv_thread_runner() {
    printf("initing recv thread\n");
    static int count = 1;
    int socket_fd;
    // 创建原始套接字
    if ((socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 绑定到指定的网络接口
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, this->name, IFNAMSIZ - 1);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Binding to network interface failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    // 捕获并打印报文
    while (true) {
        struct iphdr *ipv4_header;
        
        // 接收报文
        memset(recv_buffer, 0, BUFFER_SIZE);
        ssize_t packet_len = recv(socket_fd, recv_buffer, BUFFER_SIZE, 0);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        ipv4_header = (struct iphdr*)(recv_buffer + sizeof(struct ethhdr));
        if (ipv4_header->version != 4 
        || ipv4_header->protocol != 89 
        || (ipv4_header->daddr != this->ip && ipv4_header->daddr != inet_addr("224.0.0.5"))) {
            continue;
        }

        printf("第%d条报文\n", count++);

        //分发报文
        struct OSPFHeader *ospf_header = (struct OSPFHeader*)ipv4_header; 
        switch(ospf_header->type) {
            case OSPFPacketType::HELLO:
                handle_recv_hello((struct OSPFHello*)ospf_header, this);
                break;
            case OSPFPacketType::DD:
                handle_recv_dd((struct OSPFDD*)ospf_header);
                break;
            case OSPFPacketType::LSR:
                handle_recv_lsr((struct OSPFLSR*)ospf_header);
                break;
            case OSPFPacketType::LSU:
                handle_recv_lsu((struct OSPFLSU*)ospf_header);
                break;
            case OSPFPacketType::LSA:
                handle_recv_lsa((struct OSPFLSAck*)ospf_header);
                break;
            default:
                printf("Error: illegal type");
        }
        printf("\n\n");
    }    
}


void handle_recv_hello(OSPFHello *hello_packet, Interface *interface) {
    // 在此处理Hello报文
    hello_packet->show();
    if (hello_packet->network_mask != interface->network_mask
        || ntohs(hello_packet->hello_interval) != interface->hello_interval
        || ntohl(hello_packet->dead_interval) != interface->dead_interval
        ) {
        printf("不匹配的hello报文");
    }

    Neighbor *neighbor = interface->get_neighbor(hello_packet->header.router_id);
    if (neighbor == NULL) {
        neighbor = generate_from_hello(hello_packet, interface);
        interface->neighbors.push_back(neighbor);
    }
    neighbor->event_hello_received();
    if (neighbor->state == NeighborState::INIT && hello_packet->has_neighbor(interface->router_id)) {
        neighbor->event_2way_received();
    }
    if (hello_packet->has_neighbor(interface->router_id)
        && neighbor->state == NeighborState::INIT) {
        neighbor->state = NeighborState::_2WAY;
    }
}

void handle_recv_dd(OSPFDD *dd_packet) {
    // 在此处理DD报文
    dd_packet->show();
}

void handle_recv_lsr(OSPFLSR *lsr_packet) {
    // 在此处理LSR报文
    lsr_packet->show();
}

void handle_recv_lsu(OSPFLSU *lsu_packet) {
    // 在此处理LSU报文
    lsu_packet->show();
}

void handle_recv_lsa(OSPFLSAck *lsa_packet) {
    // 在此处理LSA报文
    lsa_packet->show();
}