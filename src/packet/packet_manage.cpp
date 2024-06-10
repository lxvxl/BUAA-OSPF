#include "../../include/packet/packet_manage.h"
#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"
#define BUFFER_SIZE 65535

char buffer[BUFFER_SIZE];

void recv_thread_runner(Interface *interface);

void create_recv_thread(Interface *interface) {
    interface->thread = std::thread(recv_thread_runner, interface);
    interface->thread.detach();
    return;
}

void recv_thread_runner(Interface *interface) {
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
        
        memset(buffer, 0, BUFFER_SIZE);
        // 接收报文
        ssize_t packet_len = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        ipv4_header = (struct iphdr*)buffer;

        show_ipv4_header(ipv4_header);
        printf("\n\n");
    }
}