#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"


#include <functional>

void recv_thread_runner(Interface *interface);
void handle_recv_hello(OSPFHello *hello_packet, Interface *interface, uint32_t saddr);
void handle_recv_dd(OSPFDD *dd_packet, Interface *interface);
void handle_recv_lsr(OSPFLSR *lsr_packet, Interface *interface);
void handle_recv_lsu(OSPFLSU *lsu_packet, Interface *interface, uint32_t saddr, uint32_t daddr);
void handle_recv_lsack(OSPFLSAck *lsa_packet, Interface *interface);


void Interface::recv_thread_runner() {
    logger::other_log(this, "initing recv thread");
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
    while (state != DOWN) {
        // 接收报文
        memset(recv_buffer, 0, BUFFER_SIZE);
        ssize_t packet_len = recv(this->recv_socket_fd, recv_buffer, BUFFER_SIZE, 0);
        std::unique_lock<std::mutex> lock(router::mutex);
        if (packet_len < 0) {
            perror("Failed to receive packets");
            exit(EXIT_FAILURE);
        }

        struct iphdr *ipv4_header = (struct iphdr*)(recv_buffer + sizeof(struct ethhdr));
        if (ipv4_header->version != 4 
        || ipv4_header->protocol != 89 
        || (ipv4_header->daddr != this->ip && ipv4_header->daddr != inet_addr("224.0.0.5") && (ipv4_header->daddr != inet_addr("224.0.0.6")))) {
            //if (ipv4_header->version == 4 && ipv4_header->protocol == 1) {
            //    uint32_t daddr = ipv4_header->daddr;
            //    Interface *target_interface = router::routing_table.query(daddr);
            //    if (target_interface == NULL) {
            //        logger::other_log(this, "Unknown address: " + std::string(inet_ntoa({daddr})));
            //    } else if (target_interface != this && ipv4_header->ttl != 1) {
            //        logger::other_log(this, "dst address: " + std::string(inet_ntoa({daddr})));
            //        target_interface->transmit_packet(recv_buffer + sizeof(struct ethhdr), ntohs(ipv4_header->tot_len));
            //    }
            //}
            continue;
        }
        if ((ipv4_header->daddr == inet_addr("224.0.0.6")) && (this->state != DR && this->state != BACKUP)) {
            continue;
        }
        if ((ipv4_header->saddr & this->network_mask) != (this->ip & this->network_mask)) {
            continue;//h不知道s为什么会受到其他网卡的东西
        }
        //logger::other_log(this, "received a packet");
        
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
                handle_recv_lsu((struct OSPFLSU*)ospf_header, this, saddr, ipv4_header->daddr);
                break;
            case OSPFPacketType::LSA:
                handle_recv_lsack((struct OSPFLSAck*)ospf_header, this);
                break;
            default:
                logger::other_log(this, "unmatched packet type");
        }
        lock.unlock();
    }    
    logger::event_log(this, "接收线程已关闭");
}


void handle_recv_hello(OSPFHello *hello_packet, Interface *interface, uint32_t saddr) {
    // 在此处理Hello报文
    if (hello_packet->network_mask != interface->network_mask
        || ntohs(hello_packet->hello_interval) != interface->hello_interval
        || ntohl(hello_packet->dead_interval) != interface->dead_interval
        ) {
        logger::other_log(interface, "unmatched hello packet");
    }

    Neighbor *neighbor = interface->get_neighbor_by_id(hello_packet->header.router_id);
    if (neighbor == NULL) {
        neighbor = new Neighbor(hello_packet, interface, saddr);
        interface->neighbors.push_back(neighbor);
    }
    neighbor->event_hello_received();

    //若邻居的hello包中有自己
    if (hello_packet->has_neighbor(router::router_id)) {
        neighbor->event_2way_received();
    } else {
        neighbor->event_1way_received();
        return;
    }
    //如果邻居宣告自己为 DR，且宣告BDR为 0.0.0.0，且接收接口状态机的状态为 Waiting，执行事件BackupSeen。
    if (hello_packet->designated_router == neighbor->ip 
            && hello_packet->backup_designated_router == inet_addr("0.0.0.0") 
            && interface->state == WAITING) {
        interface->event_backup_seen();
        return;
    }
    //否则，如果以前不宣告的邻居宣告自己为 DR，或以前宣告的邻居现在不宣告自己为 DR，接收接口状态机调度执行事件NeighborChange。
    if ((hello_packet->designated_router == neighbor->ip) ^ (neighbor->dr == neighbor->ip)) {
        interface->event_neighbor_change();
        return;
    }
    //如果邻居宣告自己为 BDR，且接收接口状态机的状态为 Waiting，接收接口状态机调度执行事件 BackupSeen。
    if (hello_packet->backup_designated_router == neighbor->ip 
            && interface->state == WAITING) {
        interface->event_backup_seen();
        return;
    }
    //否则，如果以前不宣告的邻居宣告自己为 BDR，或以前宣告的邻居现在不宣告自己 BDR，接收接口状态机调度执行事件 NeighborChange。
    if ((hello_packet->backup_designated_router == neighbor->ip) ^ (neighbor->bdr == neighbor->ip)) {
        interface->event_neighbor_change();
        return;
    }
}

void handle_recv_dd(OSPFDD *dd_packet, Interface *interface) {
    //if (ntohs(dd_packet->interface_mtu) > router::config::MTU) {
    //    return;
    //}
    // 在此处理DD报文
    //dd_packet->show();
    Neighbor *neighbor = interface->get_neighbor_by_id(dd_packet->header.router_id);
    if (neighbor == NULL) {
        return;
    }
    OSPFDD *last_dd = neighbor->dd_last_recv;
    uint32_t packet_dd_seq_num = ntohl(dd_packet->dd_sequence_number);
    switch (neighbor->state) {
        case DOWN:
        case ATTEMPT:
        case _2WAY:
            return;
        case INIT:
            neighbor->event_2way_received();
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
                logger::other_log(interface, "dd refused: repeated packet");
                return;
            } 
            memcpy(last_dd, dd_packet, 2048);
            if (dd_packet->get_lsa_num() != 0 || !dd_packet->b_I || !dd_packet->b_M || !dd_packet->b_MS) {
                logger::other_log(interface, "dd refused: wrong format during EXSTART state, lsa_num: " + std::to_string((int)dd_packet->get_lsa_num())
                                                + " b_I: " + std::to_string(dd_packet->b_MS));
                return;
            }
            //确定主从
            neighbor->event_negotiation_done();
            if (ntohl(dd_packet->header.router_id) > ntohl(router::router_id)) {
                //自己是从机
                neighbor->b_MS = 0;
                neighbor->dd_sequence_number = ntohl(dd_packet->dd_sequence_number);
                neighbor->b_I = 0;
                neighbor->dd_retransmit_timer = -1;
                interface->send_dd_packet(neighbor);
                neighbor->dd_sequence_number++;
            } else {
                //自己是主机
                neighbor->b_MS = 1;
                neighbor->b_I = 0;
            }
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
                logger::event_log(interface, "received repeated DD packet");
                return;
            } 
            //主从位不匹配或意外设置初始位或选项域与之前不同
            if (!(dd_packet->b_MS ^ neighbor->b_MS) || dd_packet->b_I || last_dd->options != dd_packet->options) {
                logger::event_log(interface, "received unmatched MS or I or options in DD packet");
                neighbor->event_seq_num_mismatch();
                return;
            }
            memcpy(last_dd, dd_packet, 2048);
            //检查dd序号
            // if (neighbor->b_MS && neighbor->dd_sequence_number != packet_dd_seq_num + 1 
            //     || !neighbor->b_MS && neighbor->dd_sequence_number != packet_dd_seq_num) {
            //     logger::event_log(interface, "received unmatched sequence number in DD packet");
            //     neighbor->event_seq_num_mismatch();
            //     return;
            // }
            if (neighbor->dd_sequence_number != packet_dd_seq_num ) {
                logger::event_log(interface, "received unmatched sequence number in DD packet");
                neighbor->event_seq_num_mismatch();
                return;
            }
            //添加没有的LSA
            for (int i = 0; i < dd_packet->get_lsa_num(); i++) {
                dd_packet->lsa_headers[i].ntoh();
                if (router::lsa_db.get_lsa(&dd_packet->lsa_headers[i]) == NULL) {
                    LSAHeader *req_header = new LSAHeader;
                    *req_header = dd_packet->lsa_headers[i];
                    neighbor->req_v_lsas.push_back(req_header);
                }
            }
            // 判断是否还需要发送
            if (neighbor->b_MS) { //如果是主机
                if (neighbor->dd_has_more_lsa() || dd_packet->b_M) {
                    neighbor->dd_sequence_number++;
                    interface->send_dd_packet(neighbor);
                } else {
                    neighbor->event_exchange_done();
                }
            } else { //如果是从机
                if (!neighbor->dd_has_more_lsa() && !dd_packet->b_M) {
                    neighbor->event_exchange_done();
                }
                interface->send_dd_packet(neighbor);
                neighbor->dd_sequence_number++;
            }
            break;
        case LOADING:
        case FULL:
            // 这时候只可能收到重复的包。如果不重复，生成seq mismatch事件。否则，从机重发报文。
            if (dd_packet->b_I) {
                neighbor->event_seq_num_mismatch();
                break;
            }
            if (dd_packet->b_I == last_dd->b_I
                    && dd_packet->b_M == last_dd->b_M
                    && dd_packet->b_MS == last_dd->b_MS
                    && dd_packet->dd_sequence_number == last_dd->dd_sequence_number) {
                if (!neighbor->b_MS) {
                    interface->send_last_dd_packet(neighbor);
                }
            } else {
                neighbor->event_seq_num_mismatch();
            }
        default:
            break;
    }
}

void handle_recv_lsr(OSPFLSR *lsr_packet, Interface *interface) {
    std::vector<LSAHeader*> req_r_lsas;
    Neighbor *neighbor = interface->get_neighbor_by_id(lsr_packet->header.router_id);
    if (neighbor == NULL) {
        return;
    }
    if (neighbor->state != EXCHANGE
            && neighbor->state != LOADING
            && neighbor->state != FULL) {
        return;
    }
    for (int i = 0; i < lsr_packet->get_req_num(); i++) {
        LSAHeader *lsa = router::lsa_db.get_lsa(ntohl(lsr_packet->reqs[i].ls_type), 
                                                lsr_packet->reqs[i].link_state_id, 
                                                lsr_packet->reqs[i].advertising_router);
        if (lsa == NULL) {
            std::cout<<"querying a null lsa\n";
            neighbor->event_bad_lsreq();
            return;
        }
        req_r_lsas.push_back(lsa);
    }
    interface->send_lsu_packet(req_r_lsas, neighbor->ip);
}

void handle_recv_lsu(OSPFLSU *lsu_packet, Interface *interface, uint32_t saddr, uint32_t daddr) {
    // 在此处理LSU报文
    LSAHeader *next_v_lsa = (LSAHeader*)((uint8_t*)lsu_packet + sizeof(OSPFLSU));
    std::vector<LSAHeader*> received_v_lsas;
    Neighbor *neighbor = interface->get_neighbor_by_ip(saddr);
    for (uint32_t i = 0; i < ntohl(lsu_packet->lsa_num); i++) {
        received_v_lsas.push_back(next_v_lsa);
        //更新数据库，跳转到下一条lsa
        switch (next_v_lsa->ls_type) {
            case LSType::ROUTER:
                ((RouterLSA*)next_v_lsa)->ntoh();
                next_v_lsa = (LSAHeader*)((uint8_t*)next_v_lsa + next_v_lsa->length);
                break;
            case LSType::NETWORK:{
                ((NetworkLSA*)next_v_lsa)->ntoh();
                next_v_lsa = (LSAHeader*)((uint8_t*)next_v_lsa + next_v_lsa->length);}
                break;
            default:
                break;
        }
    }
    for (LSAHeader* v_lsa : received_v_lsas) {
        LSADatabase& lsa_db = router::lsa_db;
        LSAHeader *r_lsa = lsa_db.get_lsa(v_lsa);
        //如果该实例比数据库中新或者数据库中不存在实例
        if (r_lsa == NULL || v_lsa->compare(r_lsa) < 0) {
            //如果该实例收到保护
            //if (lsa_db.protected_lsas.find(r_lsa) != lsa_db.protected_lsas.end()) {
            //    logger::other_log(interface, "trying to override a protected lsa");
            //    continue;
            //}
            r_lsa = lsa_db.update(v_lsa);
            //查看该实例是否是请求的LSA。如果是，将其删除
            if (daddr == interface->ip && neighbor->rm_from_reqs(v_lsa)) {
                logger::other_log(interface, "got a quried lsa");
                //v_lsa->show();
                continue;
            }
            //否则，泛洪，将其从重传列表中删除
            InterfaceState sender_state = interface->dr == saddr ? DR :
                                          interface->bdr == saddr ? BACKUP: DROTHER;
            LSADatabase::flood(r_lsa, interface, sender_state);
            neighbor->lsu_retransmit_manager.remove_lsa(r_lsa);
            interface->send_lsack_packet(v_lsa, daddr);
        } else { //数据库中存在更新的实例或者相同的实例
            if (neighbor->rm_from_reqs(v_lsa)) { //如果这个实例正在请求列表中，生成BadLSReq事件
                // std::cout<<"querying lsa:\n";
                // v_lsa->show();
                // std::cout<<"but we have lsa:\n";
                // r_lsa->show();
                if (v_lsa->compare(r_lsa) == 0) {
                    interface->send_lsack_packet(v_lsa, daddr);
                } else {
                    neighbor->event_bad_lsreq();
                }
                return;
            }
            //如果存在相同实例
            if (r_lsa->compare(v_lsa) == 0) {
                neighbor->lsu_retransmit_manager.remove_lsa(r_lsa);//隐含确认

                interface->send_lsack_packet(v_lsa, saddr);
            } else {
                //该数据库副本没有在最近MinLSArrival内被LSU发送，将其立刻发送给邻居
                interface->send_lsu_packet(r_lsa, saddr);
            }
        }
    }
}

void handle_recv_lsack(OSPFLSAck *lsack_packet, Interface *interface) {
    Neighbor *neighbor = interface->get_neighbor_by_id(lsack_packet->header.router_id);
    if (neighbor == NULL) {
        return;
    }
    for (int i = 0; i < lsack_packet->get_lsa_num(); i++) {
        lsack_packet->lsa_headers[i].ntoh();
        LSAHeader *r_lsa = router::lsa_db.get_lsa(&lsack_packet->lsa_headers[i]);
        if (r_lsa != NULL) {
            neighbor->lsu_retransmit_manager.remove_lsa(&lsack_packet->lsa_headers[i]);
        }
    }
}