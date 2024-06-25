#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"

Neighbor* Interface::get_neighbor_by_id(uint32_t router_id) {
    for (auto n : neighbors) {
        if (n->router_id == router_id) {
            return n;
        }
    }
    return NULL;
}

Neighbor *Interface::get_neighbor_by_ip(uint32_t ip) {
    for (auto n : neighbors) {
        if (n->ip == ip) {
            return n;
        }
    }
    return NULL;
}

Interface::Interface(const char *name, uint32_t ip, uint32_t mask) {
    this->name         = name;
    this->ip           = ip;
    this->network_mask = mask;
    memset(send_buffer, 0, sizeof(send_buffer));
    memset(recv_buffer, 0, sizeof(recv_buffer));
    router::interfaces.push_back(this);
}

void Interface::clear_invalid_req(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa) {
    for (auto neighbor : neighbors) {
        //清除待发送的dd包中的相应lsa。如果new_lsa不为空，则使用new_lsa来代替原有的lsa。
        for (int i = neighbor->dd_r_lsa_headers.size() - 1; i >= 0; i--) {
            //如果要清除的实例比dd列表中的实例相同或者新
            if (old_r_lsa->compare(neighbor->dd_r_lsa_headers[i]) <= 0) {
                if (new_r_lsa == NULL) {
                    neighbor->dd_r_lsa_headers.erase(neighbor->dd_r_lsa_headers.begin() + i);
                    if (neighbor->dd_recorder >= i) {
                        neighbor->dd_recorder--;
                    }
                } else {
                    neighbor->dd_r_lsa_headers[i] = new_r_lsa;
                }
            }
        }
        //清除待重传LSU中的LSA
        neighbor->lsu_retransmit_manager.remove_lsa(old_r_lsa);
    }
}

void Interface::transmit_packet(char packet[], int length) {
    logger::other_log(this, "transmit packet");
    if (this->transmit_socket_fd == 0) {
        if ((this->transmit_socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
            perror("socket creation failed");
            return;
        }
        // 绑定到指定的网络接口
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, this->name, IFNAMSIZ - 1);

        if (setsockopt(this->recv_socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
            perror("Binding to network interface failed");
            exit(EXIT_FAILURE);
        }
    }
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(0); // 端口号在IP头中
    dest_addr.sin_addr.s_addr = ((struct iphdr*)packet)->daddr;

    // 发送数据包
    if (sendto(this->transmit_socket_fd, packet, length, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto failed");
        close(this->transmit_socket_fd);
        this->transmit_socket_fd = 0;
        return;
    }
    logger::other_log(this, "successfully transmitted");
}
