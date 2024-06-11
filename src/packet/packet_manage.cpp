#include "../../include/packet/packet_manage.h"
#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"
#define BUFFER_SIZE 4096

char buffer[BUFFER_SIZE];

void recv_thread_runner(Interface *interface);
void handle_recv_hello(OSPFHello *hello_packet);
void handle_recv_dd(OSPFDD *dd_packet);
void handle_recv_lsr(OSPFLSR *lsr_packet);
void handle_recv_lsu(OSPFLSU *lsu_packet);
void handle_recv_lsa(OSPFLSA *lsa_packet);


void create_recv_thread(Interface *interface) {
    interface->thread = std::thread(recv_thread_runner, interface);
    interface->thread.detach();
    return;
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
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t packet_len = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        ipv4_header = (struct iphdr*)(buffer + sizeof(struct ethhdr));
        if (ipv4_header->version != 4 || ipv4_header->protocol != 89) {
            continue;
        }

        printf("第%d条报文\n", count++);
        show_ipv4_header(ipv4_header);
        printf("\n\n");

        //分发报文
        struct OSPFHeader *ospf_header = (struct OSPFHeader*)((char*)ipv4_header + 20); 
        switch(ospf_header->type) {
            case OSPFPacketType::HELLO:
                handle_recv_hello((struct OSPFHello*)ospf_header);
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

void handle_recv_hello(OSPFHello *hello_packet) {
    // 在此处理Hello报文
    hello_packet->show();
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