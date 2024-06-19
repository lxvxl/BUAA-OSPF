#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include <functional>

void Interface::send_thread_runner() {
    printf("initing hello thread\n");
    if ((this->send_socket_fd = socket(AF_INET, SOCK_RAW, 89)) < 0) {
        perror("[Thread]SendHelloPacket: socket_fd init");
    }
    /* Bind sockets to certain Network Interface */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, this->name);
    if (setsockopt(this->send_socket_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        perror("[Thread]SendHelloPacket: setsockopt");
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->hello_timer--;
        this->wait_timer--;

        if (this->hello_timer == 0) {
            this->hello_timer = 10;
            this->send_hello_packet();
        }

        if (this->wait_timer == 0 && this->state == InterfaceState::WAITING) {
            this->event_wait_timer();
        }

        for (auto neighbor : this->neighbors) {
            neighbor->inactivity_timer--;
            if (neighbor->inactivity_timer == 0) {
                neighbor->event_kill_nbr();
            }

            //查看dd报文重发
            neighbor->dd_retransmit_timer--;
            if (neighbor->dd_retransmit_timer == 0) {
                this->send_last_dd_packet(neighbor);
            }

            //查看LSR报文重传
            neighbor->lsr_retransmit_timer--;
            if (neighbor->lsr_retransmit_timer == 0 && !neighbor->req_v_lsas.empty()) {
                this->send_lsr_packet(neighbor);
            }

            //查看LSU报文重传
            neighbor->lsu_retransmit_manager.step_one();
            std::vector<LSAHeader*> retransmit_r_lsas;
            neighbor->lsu_retransmit_manager.get_retransmit_lsas(retransmit_r_lsas);
            if (!retransmit_r_lsas.empty()) {
                send_lsu_packet(retransmit_r_lsas, neighbor->ip);
            }
        }
    }
}

void Interface::send_hello_packet() {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = inet_addr("224.0.0.5");
    ((OSPFHello*)this->send_buffer)->fill(this);
    if (sendto(this->send_socket_fd, 
               this->send_buffer, 
               44 + this->neighbors.size() * 4, 
               0, 
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]SendHelloPacket: sendto");
    } 
    //printf("[hello thread] send a packet");
}

void Interface::send_dd_packet(Neighbor *neighbor) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = neighbor->ip;
    ((OSPFDD*)this->send_buffer)->fill(neighbor);
    if (sendto(this->send_socket_fd, 
               this->send_buffer, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]SendHelloPacket: sendto");
    } 
    memcpy(neighbor->dd_last_send, this->send_buffer, 2048);
    if (neighbor->is_master) {
        neighbor->dd_retransmit_timer = 10;
    }
}

void Interface::send_last_dd_packet(Neighbor *neighbor) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = neighbor->ip;
    if (sendto(this->send_socket_fd, 
               neighbor->dd_last_send, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]SendHelloPacket: sendto");
    } 
    if (neighbor->is_master) {
        neighbor->dd_retransmit_timer = 10;
    }
}

void Interface::send_lsr_packet(Neighbor *neighbor) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = neighbor->ip;

    ((OSPFLSR*)this->send_buffer)->fill(neighbor->req_v_lsas, this);
    if (sendto(this->send_socket_fd, 
               this->send_buffer, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, 
               sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]Send: sendto");
    } 
}


void Interface::send_lsu_packet(std::vector<LSAHeader*>& r_lsas, uint32_t dst_addr) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = dst_addr;

    ((OSPFLSU*)this->send_buffer)->fill(r_lsas, this);
    if (sendto(this->send_socket_fd, 
               this->send_buffer, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, 
               sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]Send: sendto");
    } 
    if (dst_addr != inet_addr("224.0.0.5")) {
        Neighbor *neighbor = this->get_neighbor_by_ip(dst_addr);
        for (auto lsa : r_lsas) {
            neighbor->lsu_retransmit_manager.add_lsa(lsa);
        }
    }
}

void Interface::send_lsack_packet(std::vector<LSAHeader*>& v_lsas, uint32_t dst_addr) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = dst_addr;

    ((OSPFLSAck*)this->send_buffer)->fill(v_lsas, this);
    if (sendto(this->send_socket_fd, 
               this->send_buffer, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, 
               sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]Send: sendto");
    }
}

