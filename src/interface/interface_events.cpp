#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/logger/logger.h"
#include "../../include/global_settings/router.h"
#include <functional>
#define event_pre_aspect InterfaceState pre_state = this->state;
#define event_post_aspect if (pre_state != state) {\
    logger::state_transition_log(this, pre_state, this->state);router::lsa_db.generate_router_lsa();}

/**
 * 启动接口
 * Down --> Point-to-Point | DR Other | Waiting
 */
void Interface::event_interface_up() {
    logger::event_log(this, "interface up");
    event_pre_aspect

    this->hello_timer = 1;
    //TODO：默认广播或NBMA网络
    if (this->rtr_priority == 0) {
        this->state = DROTHER;
    } else {
        this->state = WAITING;
        this->wait_timer = this->dead_interval;
    }
    
    this->main_thread = std::thread(std::bind(&Interface::main_thread_runner, this));
    this->main_thread.detach();
    this->recv_thread = std::thread(std::bind(&Interface::recv_thread_runner, this));
    this->recv_thread.detach();
    event_post_aspect
}

/**
 * 等待记时器到期，表示在选举 DR 和 BDR 前的等待状态已经结束。
 */
void Interface::event_wait_timer() {
    logger::event_log(this, "wait timer");
    event_pre_aspect

    if (this->state != InterfaceState::WAITING) {
        goto end;
    }

    elect_dr();
    if (this->ip == this->dr) {
        this->state = InterfaceState::DR;
    } else if (this->ip == this->bdr) {
        this->state = InterfaceState::BACKUP;
    } else {
        this->state = InterfaceState::DROTHER;
    }
end:
    event_post_aspect
}

/**
 * 路由器已经探知在网络上是否存在 BDR
 */
void Interface::event_backup_seen() {
    logger::event_log(this, "backup seen");
    event_pre_aspect

    if (this->state != InterfaceState::WAITING) {
        goto end;
    }

    elect_dr();
    if (this->ip == this->dr) {
        this->state = InterfaceState::DR;
    } else if (this->ip == this->bdr) {
        this->state = InterfaceState::BACKUP;
    } else {
        this->state = InterfaceState::DROTHER;
    }
    this->wait_timer = -1;
end:
    event_post_aspect
}

void Interface::event_neighbor_change() {
    if (this->state < DROTHER) {
        return;
    }
    logger::event_log(this, "neighbor change");
    event_pre_aspect

    elect_dr();
    if (this->ip == this->dr) {
        this->state = InterfaceState::DR;
    } else if (this->ip == this->bdr) {
        this->state = InterfaceState::BACKUP;
    } else {
        this->state = InterfaceState::DROTHER;
    }
    this->wait_timer = -1;
    event_post_aspect
}

void Interface::event_loop_ind() {
    logger::event_log(this, "loop ind");
    event_pre_aspect
    this->state = LOOPBACK;
    event_post_aspect
}

void Interface::event_unloop_ind() {
    logger::event_log(this, "unloop ind");
    event_pre_aspect
    this->state = DOWN;
    event_post_aspect
}

void Interface::event_interface_down() {
    logger::event_log(this, "interface down");
    event_pre_aspect
    this->state = DOWN;
    for (Neighbor* neighbor : neighbors) {
        delete neighbor;
    }
    neighbors.clear();
    event_post_aspect
}

bool is_bdr_or_dr(uint32_t ip, uint32_t dr, uint32_t bdr) {
    return ip == dr || ip == bdr;
}

void Interface::elect_dr() {
    // Step 1: Create a list of neighbors in FULL state with non-zero priority
    logger::other_log(this, "elect DR and BDR");
    std::vector<Neighbor*> candidates;
    for (auto neighbor : neighbors) {
        if (neighbor->state >= NeighborState::_2WAY && neighbor->priority > 0) {
            candidates.push_back(neighbor);
        }
    }

    // Include this router in the election if its priority is non-zero
    if (rtr_priority > 0) {
        Neighbor local_router;
        local_router.state = NeighborState::_2WAY;
        local_router.router_id = router::router_id;
        local_router.priority = rtr_priority;
        local_router.ip = ip;
        local_router.dr = dr;
        local_router.bdr = bdr;
        candidates.push_back(&local_router);
    }

    // Step 2: Sort the candidates based on priority and router_id
    std::sort(candidates.begin(), candidates.end(), [](Neighbor* a, Neighbor* b) {
        if (a->priority != b->priority)
            return a->priority > b->priority;
        return ntohl(a->router_id) > ntohl(b->router_id);
    });

    // Step 3: Select the DR and BDR
    uint32_t new_dr = 0;
    uint32_t new_bdr = 0;

    //选举BDR
    //若有人宣告自己是BDR，选取其中优先度最高的，宣告自己是DR的不参与
    for (auto candidate : candidates) {
        if (candidate->dr == candidate->ip) {
            continue;
        }
        if (candidate->bdr == candidate->ip) {
            new_bdr = candidate->ip;
            break;
        }
    }
    //若没人宣告自己是BDR，选取优先级最高的，同样排除宣告自己为DR的路由器
    if (new_bdr == 0) {
        for (auto candidate : candidates) {
            if (candidate->dr == candidate->ip) {
                continue;
            }
            new_bdr = candidate->ip;
            break;
        }
    }

    //选举DR
    //先从宣告自己是DR的接口里面找
    for (auto candidate : candidates) {
        if (candidate->dr == candidate->ip) {
            new_dr = candidate->ip;
            break;        
        }
    }
    if (new_dr == 0) {
        new_dr = new_bdr;
        new_bdr = 0;
        for (auto candidate : candidates) {
            if (new_dr == candidate->ip) {
                continue;
            }
            new_bdr = candidate->ip;
            break;
        }    
    }
    ////如果路由器 X 新近成为 DR 或 BDR，或者不再成为 DR 或 BDR，重复步骤 2 和 3
    //if (is_bdr_or_dr(ip, dr, bdr) ^ is_bdr_or_dr(ip, new_dr, new_bdr)) {
    //    if (ip == new_dr) {
    //        
    //    }
    //}

    // Update interface DR and BDR
    dr = new_dr;
    bdr = new_bdr;

    // Print the results for debugging purposes
    std::string log_str = "New DR: ";
    log_str += inet_ntoa({new_dr});
    log_str += ", New BDR: ";
    log_str += inet_ntoa({new_bdr});
    logger::other_log(this, log_str);

    for (auto neighbor : this->neighbors) {
        if (neighbor->state >= _2WAY) {
            neighbor->event_is_adj_ok();
        }
    }
}