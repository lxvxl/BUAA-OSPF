#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/router.h"

#include <functional>

void recv_thread_runner(Interface *interface);
void handle_recv_hello(OSPFHello *hello_packet, Interface *interface, uint32_t saddr);
void handle_recv_dd(OSPFDD *dd_packet, Interface *interface);
void handle_recv_lsr(OSPFLSR *lsr_packet, Interface *interface);
void handle_recv_lsu(OSPFLSU *lsu_packet, Interface *interface, uint32_t saddr);
void handle_recv_lsack(OSPFLSAck *lsa_packet, Interface *interface);


void Interface::recv_thread_runner() {
    printf("initing recv thread\n");
    // 创建原始套接字
    if ((this->recv_socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 绑定到指定的网络接口
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, this->name, IFNAMSIZ - 1);

    if (setsockopt(this->recv_socket_fd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        perror("Binding to network interface failed");
        exit(EXIT_FAILURE);
    }
    // 捕获并打印报文
    while (true) {
        
        // 接收报文
        memset(recv_buffer, 0, BUFFER_SIZE);
        ssize_t packet_len = recv(this->recv_socket_fd, recv_buffer, BUFFER_SIZE, 0);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            exit(EXIT_FAILURE);
        }

        struct iphdr *ipv4_header = (struct iphdr*)(recv_buffer + sizeof(struct ethhdr));
        if (ipv4_header->version != 4 
        || ipv4_header->protocol != 89 
        || (ipv4_header->daddr != this->ip && ipv4_header->daddr != inet_addr("224.0.0.5"))) {
            continue;
        }
        
        struct OSPFHeader *ospf_header = (struct OSPFHeader*)(recv_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr)); 
        uint32_t saddr = ipv4_header->saddr;

        //分发报文
        switch(ospf_header->type) {
            case OSPFPacketType::HELLO:
                handle_recv_hello((struct OSPFHello*)ospf_header, this, saddr);
                break;
            case OSPFPacketType::DD:
                handle_recv_dd((struct OSPFDD*)ospf_header, this);
                break;
            case OSPFPacketType::LSR:
                handle_recv_lsr((struct OSPFLSR*)ospf_header, this);
                break;
            case OSPFPacketType::LSU:
                handle_recv_lsu((struct OSPFLSU*)ospf_header, this, saddr);
                break;
            case OSPFPacketType::LSA:
                handle_recv_lsack((struct OSPFLSAck*)ospf_header, this);
                break;
            default:
                printf("Error: illegal type");
        }
    }    
}


void handle_recv_hello(OSPFHello *hello_packet, Interface *interface, uint32_t saddr) {
    // 在此处理Hello报文
    if (hello_packet->network_mask != interface->network_mask
        || ntohs(hello_packet->hello_interval) != interface->hello_interval
        || ntohl(hello_packet->dead_interval) != interface->dead_interval
        ) {
        printf("不匹配的hello报文");
    }

    Neighbor *neighbor = interface->get_neighbor_by_id(hello_packet->header.router_id);
    if (neighbor == NULL) {
        neighbor = new Neighbor(hello_packet, interface, saddr);
        interface->neighbors.push_back(neighbor);
    }
    neighbor->event_hello_received();

    //若邻居的hello包中有自己
    if (neighbor->state == NeighborState::INIT && hello_packet->has_neighbor(router::router_id)) {
        neighbor->event_2way_received(interface);
    }
    //查看是否需要选举dr和bdr
    if (hello_packet->backup_designated_router == saddr || (hello_packet->designated_router == saddr && hello_packet->backup_designated_router == 0)) {
        interface->event_backup_seen();
    }
}

void handle_recv_dd(OSPFDD *dd_packet, Interface *interface) {
    //if (ntohs(dd_packet->interface_mtu) > router::config::MTU) {
    //    return;
    //}
    // 在此处理DD报文
    printf("dd!\n");
    Neighbor *neighbor = interface->get_neighbor_by_id(dd_packet->header.router_id);
    OSPFDD *last_dd = neighbor->dd_last_recv;
    uint32_t packet_dd_seq_num = ntohl(dd_packet->dd_sequence_number);
    switch (neighbor->state) {
        case DOWN:
        case ATTEMPT:
        case _2WAY:
            return;
        case INIT:
            neighbor->event_2way_received(interface);
            if (neighbor->state == _2WAY) {
                return;
            }
            //进入EXSTART
        case EXSTART:
            // 查看dd报文是否有效，如果和之前的一样，则丢弃这个dd包
            if (dd_packet->b_I == last_dd->b_I
                    && dd_packet->b_M == last_dd->b_M
                    && dd_packet->b_MS == last_dd->b_MS
                    && dd_packet->dd_sequence_number == last_dd->dd_sequence_number) {
                printf("dd refused: repeated paket\n");
                return;
            } 
            memcpy(last_dd, dd_packet, 2048);
            if (dd_packet->get_lsa_num() != 0 || !dd_packet->b_I || !dd_packet->b_M || !dd_packet->b_MS) {
                std::cout<<"dd refused: exchange wrong format"<<dd_packet->get_lsa_num()<<std::endl;
                return;
            }
            //确定主从
            if (ntohl(dd_packet->header.router_id) > ntohl(router::router_id)) {
                neighbor->b_MS = 0;
                neighbor->dd_sequence_number = ntohl(dd_packet->dd_sequence_number);
                neighbor->b_I = 0;
                neighbor->dd_retransmit_timer = -1;
                interface->send_dd_packet(neighbor);
            } else {
                neighbor->b_MS = 1;
                neighbor->b_I = 0;
            }
            neighbor->event_negotiation_done();
            break;
        case EXCHANGE:
            //主机丢弃所收到的重复 DD 包；从机收到重复的 DD 包时，则应当重发前一个 DD 包
            if (dd_packet->b_I == last_dd->b_I
                    && dd_packet->b_M == last_dd->b_M
                    && dd_packet->b_MS == last_dd->b_MS
                    && dd_packet->dd_sequence_number == last_dd->dd_sequence_number) {
                if (!neighbor->b_MS) {
                    interface->send_last_dd_packet(neighbor);
                }
                return;
            } 
            //主从位不匹配或意外设置初始位或选项域与之前不同
            if (!(dd_packet->b_MS ^ neighbor->b_MS) || dd_packet->b_I || last_dd->options != dd_packet->options) {
                neighbor->event_seq_num_mismatch();
                return;
            }
            memcpy(last_dd, dd_packet, 2048);
            //检查dd序号
            if (neighbor->b_MS && neighbor->dd_sequence_number != packet_dd_seq_num 
                || !neighbor->b_MS && neighbor->dd_sequence_number + 1 != packet_dd_seq_num) {
                neighbor->event_seq_num_mismatch();
                return;
            }
            //添加没有的LSA
            for (int i = 0; i < dd_packet->get_lsa_num(); i++) {
                dd_packet->lsa_headers[i].ntoh();
                if (!router::lsa_db.has_lsa(dd_packet->lsa_headers + i)) {
                    LSAHeader *req_header = new LSAHeader;
                    *req_header = dd_packet->lsa_headers[i];
                    neighbor->req_v_lsas.push_back(req_header);
                }
            }
            // 判断是否还需要发送
            if (neighbor->b_MS) {
                if (neighbor->dd_has_more_lsa() || dd_packet->b_M) {
                    interface->send_dd_packet(neighbor);
                } else {
                    neighbor->event_exchange_done();
                }
            } else {
                if (!neighbor->dd_has_more_lsa() && !dd_packet->b_M) {
                    neighbor->event_exchange_done();
                }
                interface->send_dd_packet(neighbor);
            }
            break;
        case LOADING:
        case FULL:
        default:
            break;
    }
}

void handle_recv_lsr(OSPFLSR *lsr_packet, Interface *interface) {
    std::vector<LSAHeader*> req_r_lsas;
    Neighbor *neighbor = interface->get_neighbor_by_id(lsr_packet->header.router_id);
    for (int i = 0; i < lsr_packet->get_req_num(); i++) {
        LSAHeader *lsa = router::lsa_db.get_lsa(ntohl(lsr_packet->reqs[i].ls_type), 
                                                lsr_packet->reqs[i].link_state_id, 
                                                lsr_packet->reqs[i].advertising_router);
        if (lsa == NULL) {
            neighbor->event_bad_lsreq();
            return;
        }
        req_r_lsas.push_back(lsa);
    }
    interface->send_lsu_packet(req_r_lsas, neighbor->ip);
    
    // 在此处理LSR报文
    //lsr_packet->show();
}

void handle_recv_lsu(OSPFLSU *lsu_packet, Interface *interface, uint32_t saddr) {
    // 在此处理LSU报文
    LSAHeader *next_v_lsa = (LSAHeader*)((uint8_t*)lsu_packet + sizeof(OSPFLSU));
    std::vector<LSAHeader*> received_v_lsas;
    for (int i = 0; i < ntohl(lsu_packet->lsa_num); i++) {
        //清除待请求的lsa
        for (auto neighbor : interface->neighbors) {
            for (int j = neighbor->req_v_lsas.size() - 1; j >= 0; j--) {
                LSAHeader::Relation rel = neighbor->req_v_lsas[j]->compare(next_v_lsa);
                if (rel == LSAHeader::OLDER || rel == LSAHeader::SAME) {
                    delete neighbor->req_v_lsas[j];
                    neighbor->req_v_lsas.erase(neighbor->req_v_lsas.begin() + j);
                }
            }
        }
        received_v_lsas.push_back(next_v_lsa);
        //更新数据库，跳转到下一条lsa
        switch (next_v_lsa->ls_type) {
            case LSType::ROUTER:
                ((RouterLSA*)next_v_lsa)->ntoh();
                router::lsa_db.update(next_v_lsa);
                next_v_lsa = (LSAHeader*)((uint8_t*)next_v_lsa + next_v_lsa->length);
                break;
            case LSType::NETWORK:
                ((NetworkLSA*)next_v_lsa)->ntoh();
                router::lsa_db.update(next_v_lsa);
                next_v_lsa = (LSAHeader*)((uint8_t*)next_v_lsa + next_v_lsa->length);
                break;
            default:
                break;
        }
    }
    interface->send_lsack_packet(received_v_lsas, saddr);
}

void handle_recv_lsack(OSPFLSAck *lsack_packet, Interface *interface) {
    Neighbor *neighbor = interface->get_neighbor_by_id(lsack_packet->header.router_id);
    for (int i = 0; i < lsack_packet->get_lsa_num(); i++) {
        lsack_packet->lsa_headers[i].ntoh();
        neighbor->lsu_retransmit_manager.remove_lsa(&lsack_packet->lsa_headers[i]);
    }
}