#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/logger/logger.h"
#include "../../include/global_settings/router.h"
#include <functional>
#include <mutex>

void Interface::send_thread_runner() {
    logger::other_log(this, "initing hello thread");
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

    while (state != DOWN) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock<std::mutex> lock(router::mutex);
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
            neighbor->dd_manager.timer--;
            if (neighbor->dd_manager.timer == 0 && (neighbor->state == EXSTART || neighbor->state == EXCHANGE)) {
                this->send_last_dd_packet(neighbor);
            }

            //查看LSR报文重传
            if (neighbor->state == LOADING || neighbor->state == EXCHANGE) {
                neighbor->lsr_manager.timer--;
                if (neighbor->lsr_manager.req_v_lsas.empty()) {
                    neighbor->event_loading_done();
                } else if (neighbor->lsr_manager.timer == 0) {
                    this->send_lsr_packet(neighbor);
                    neighbor->lsr_manager.timer = this->rxmt_interval;
                }
            }

            //查看LSU报文重传
            neighbor->lsu_retransmit_manager.step_one();
            std::vector<LSAHeader*> retransmit_r_lsas;
            neighbor->lsu_retransmit_manager.get_retransmit_lsas(retransmit_r_lsas);
            if (!retransmit_r_lsas.empty()) {
                send_lsu_packet(retransmit_r_lsas, neighbor->ip);
            }
        }
        lock.unlock();
    }
    logger::event_log(this, "发送线程已关闭");
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
    //std::cout<<"send a dd\n";
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
    auto &dd_manager = neighbor->dd_manager;
    memcpy(dd_manager.last_send, this->send_buffer, 4096);
    if (dd_manager.b_MS) {
        dd_manager.timer = rxmt_interval;
    }
}

void Interface::send_last_dd_packet(Neighbor *neighbor) {
    //std::cout<<"resend a dd\n";
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = neighbor->ip;
    if (sendto(this->send_socket_fd, 
               neighbor->dd_manager.last_send, 
               ntohs(((OSPFHeader*)this->send_buffer)->packet_length), 
               0, 
               (struct sockaddr*)&dst_sockaddr, sizeof(dst_sockaddr)) < 0) {
        perror("[Thread]SendHelloPacket: sendto");
    } 
    auto &dd_manager = neighbor->dd_manager;
    if (dd_manager.b_MS) {
        dd_manager.timer = rxmt_interval;
    }
}

void Interface::send_lsr_packet(Neighbor *neighbor) {
    struct sockaddr_in dst_sockaddr;
    memset(&dst_sockaddr, 0, sizeof(dst_sockaddr));
    dst_sockaddr.sin_family = AF_INET;
    dst_sockaddr.sin_addr.s_addr = neighbor->ip;

    ((OSPFLSR*)this->send_buffer)->fill(neighbor->lsr_manager.req_v_lsas, this);
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
    //if (dst_addr != inet_addr("224.0.0.5")) {
    //    Neighbor *neighbor = this->get_neighbor_by_ip(dst_addr);
    //    for (auto lsa : r_lsas) {
    //        neighbor->lsu_retransmit_manager.add_lsa(lsa);
    //    }
    //}
}

void Interface::send_lsu_packet(LSAHeader *r_lsa, uint32_t dst_addr) {
    std::vector<LSAHeader*> r_lsas;
    r_lsas.push_back(r_lsa);
    send_lsu_packet(r_lsas, dst_addr);
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

void Interface::send_lsack_packet(LSAHeader *v_lsa, uint32_t dst_addr) {
    std::vector<LSAHeader*> v_lsas;
    v_lsas.push_back(v_lsa);
    send_lsack_packet(v_lsas, dst_addr);
}


