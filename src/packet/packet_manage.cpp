#include "../../include/packet/packet_manage.h"
#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"
#include "../../include/interface/interface.h"
#define BUFFER_SIZE 4096

char send_buffer[BUFFER_SIZE];
char recv_buffer[BUFFER_SIZE];

void recv_thread_runner(Interface *interface);
void handle_recv_hello(OSPFHello *hello_packet, Interface *interface);
void handle_recv_dd(OSPFDD *dd_packet);
void handle_recv_lsr(OSPFLSR *lsr_packet);
void handle_recv_lsu(OSPFLSU *lsu_packet);
void handle_recv_lsa(OSPFLSA *lsa_packet);

void hello_thread_runner(Interface *interface);

void create_hello_thread(Interface *interface) {
    interface->hello_thread = std::thread(hello_thread_runner, interface);
    interface->rcv_thread.detach();
}

void create_recv_thread(Interface *interface) {
    interface->rcv_thread = std::thread(recv_thread_runner, interface);
    interface->rcv_thread.detach();
    return;
}

void hello_thread_runner(Interface *interface) {
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_RAW, 89)) < 0) {
        perror("[Thread]SendHelloPacket: socket_fd init");
    }

    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface->name);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]SendHelloPacket: setsockopt");
    }

    /* Set Address : multicast */
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = inet_addr("224.0.0.5");

    while (true) {
        interface->generate_hello_packet((struct OSPFHello*)send_buffer);
        /* Send Packet */
        if (sendto(socket_fd, send_buffer + 20, 44 + interface->neighbors.size() * 4, 0, (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
            perror("[Thread]SendHelloPacket: sendto");
        } 
        std::this_thread::sleep_for(std::chrono::seconds(interface->hello_interval));
    }
}


void recv_thread_runner(Interface *interface) {
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
    strncpy(ifr.ifr_name, interface->name, IFNAMSIZ - 1);

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
        if (ipv4_header->version != 4 || ipv4_header->protocol != 89) {
            continue;
        }

        printf("第%d条报文\n", count++);
        show_ipv4_header(ipv4_header);
        printf("\n\n");

        //分发报文
        struct OSPFHeader *ospf_header = (struct OSPFHeader*)ipv4_header; 
        switch(ospf_header->type) {
            case OSPFPacketType::HELLO:
                handle_recv_hello((struct OSPFHello*)ospf_header, interface);
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
                handle_recv_lsa((struct OSPFLSA*)ospf_header);
                break;
            default:
                printf("Error: illegal type");
        }
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
        neighbor = generate_from_hello(hello_packet);
        neighbor->state = NeighborState::INIT;
        interface->neighbors.push_back(neighbor);
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

void handle_recv_lsa(OSPFLSA *lsa_packet) {
    // 在此处理LSA报文
    lsa_packet->show();
}