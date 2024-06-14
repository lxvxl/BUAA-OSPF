#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/logger/logger.h"
#include <functional>
#define mark_state InterfaceState pre_state = this->state;
#define print_state_log logger::state_transition_log(this, pre_state, this->state);

/**
 * 启动接口
 * Down --> Point-to-Point | DR Other | Waiting
 */
void Interface::event_interface_up() {
    logger::event_log(this, "interface up");
    mark_state

    this->hello_timer = 1;
    
    this->send_thread = std::thread(std::bind(&Interface::send_thread_runner, this));
    this->send_thread.detach();
    this->recv_thread = std::thread(std::bind(&Interface::recv_thread_runner, this));
    this->recv_thread.detach();

    //TODO：默认广播或NBMA网络
    if (this->rtr_priority == 0) {
        this->state = DROTHER;
    } else {
        this->state = WAITING;
        this->wait_timer = this->dead_interval;
    }
    print_state_log
}

/**
 * 等待记时器到期，表示在选举 DR 和 BDR 前的等待状态已经结束。
 */
void Interface::event_wait_timer() {
    logger::event_log(this, "wait timer");
    mark_state

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
    print_state_log
}

/**
 * 路由器已经探知在网络上是否存在 BDR
 */
void Interface::event_backup_seen() {
    logger::event_log(this, "wait timer");
    mark_state

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
    print_state_log
}

void Interface::event_neighbor_change() {
    
}

void Interface::event_loop_ind() {

}

void Interface::event_unloop_ind() {

}

void Interface::event_interface_down() {

}

void Interface::elect_dr() {
    // Step 1: Create a list of neighbors in FULL state with non-zero priority
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
        local_router.router_id = router_id;
        local_router.priority = rtr_priority;
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

    // First pass: select BDR (highest priority, then highest router_id)
    for (auto candidate : candidates) {
        if (candidate->dr != candidate->router_id) {
            new_bdr = candidate->router_id;
            break;
        }
    }

    // Second pass: select DR (highest priority, then highest router_id)
    for (auto candidate : candidates) {
        if (candidate->dr == candidate->router_id) {
            new_dr = candidate->router_id;
            break;
        }
    }

    // If no DR was found in the second pass, promote BDR to DR
    if (new_dr == 0) {
        new_dr = new_bdr;
        new_bdr = 0;

        // Find new BDR among remaining candidates
        for (auto candidate : candidates) {
            if (candidate->router_id != new_dr) {
                new_bdr = candidate->router_id;
                break;
            }
        }
    }

    // Update interface DR and BDR
    dr = new_dr;
    bdr = new_bdr;

    // Print the results for debugging purposes
    std::cout << "New DR: " << inet_ntoa({new_dr}) << ", New BDR: " << inet_ntoa({new_bdr}) << std::endl;

    for (auto neighbor : this->neighbors) {
        neighbor->event_is_adj_ok();
    }
}