#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#define BUFFER_SIZE 65536

const char *

int main() {
    int socket_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);

    // 创建原始套接字
    if ((socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 绑定到指定的网络接口enp0s3
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Binding to network interface failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    // 捕获并打印报文
    while (true) {
        // 接收报文
        ssize_t packet_len = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, &saddr, &saddr_len);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            close(socket_fd);
            exit(EXIT_FAILURE);
        }

        // 打印报文内容
        std::cout << "Received packet of length: " << packet_len << std::endl;
        for (ssize_t i = 0; i < packet_len; ++i) {
            printf("%02x ", static_cast<unsigned char>(buffer[i]));
            if ((i + 1) % 16 == 0) {
                printf("\n");
            }
        }
        printf("\n\n");
    }

    close(socket_fd);
    return 0;
}
