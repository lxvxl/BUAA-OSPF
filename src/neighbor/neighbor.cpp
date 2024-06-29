#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"
#include "../../include/db/lsa_db.h"

#define event_pre_aspect NeighborState pre_state = this->state;
#define event_post_aspect if (pre_state != this->state) {\
    logger::state_transition_log(this, pre_state, this->state);\
        if (this->state == FULL || pre_state == FULL) {\
            router::lsa_db.generate_router_lsa();\
            if (this->interface->state == DR) {\
                router::lsa_db.generate_network_lsa(this->interface);\
            }\
        }\
    }

Neighbor::Neighbor(OSPFHello *hello_packet, Interface *interface, uint32_t ip) {
    this->router_id = hello_packet->header.router_id;
    this->dr        = 0;
    this->bdr       = 0;
    this->priority  = hello_packet->rtr_priority;
    this->ip        = ip;
    this->inactivity_timer = -1;
    this->dd_manager.timer = -1;
    this->lsr_manager.timer = -1;
    this->interface = interface;
}

Neighbor::Neighbor() {
}

Neighbor::~Neighbor() {
}


void Neighbor::event_hello_received() {
    logger::event_log(this, "hello_received");
    event_pre_aspect

    if (this->state == NeighborState::N_DOWN) {
        this->inactivity_timer = this->interface->dead_interval;
        this->state = INIT;
    } else {
        this->inactivity_timer = this->interface->dead_interval;
    }

    event_post_aspect
}

void Neighbor::event_start() {
    //NBMA网络暂时不管
}   

void Neighbor::event_2way_received() {
    if (this->state != INIT) {
        return;
    }
    logger::event_log(this, "2way_received");
    event_pre_aspect

    if (this->interface->dr == this->interface->ip
            || this->interface->bdr == this->interface->ip
            || this->interface->dr == this->ip
            || this->interface->bdr == this->ip) {
        dd_manager.reset();
        state = EXSTART;
        interface->send_dd_packet(this);
    } else {
        this->state = _2WAY;
    }
    this->interface->event_neighbor_change();

end:
    event_post_aspect
}

void Neighbor::event_negotiation_done() {
    logger::event_log(this, "negotiation_done");
    event_pre_aspect

    dd_manager.prepare();

    lsr_manager.clear();

    this->state = NeighborState::EXCHANGE;
    event_post_aspect
}

void Neighbor::event_exchange_done() {
    logger::event_log(this, "exchange done");
    event_pre_aspect

    lsr_manager.timer = -1;
    if (this->lsr_manager.req_v_lsas.size() == 0) {
        this->state = FULL;
        this->lsr_manager.timer = -1;
    } else {
        this->state = LOADING;
        this->lsr_manager.timer = 1;
    }

    event_post_aspect
}   

void Neighbor::event_bad_lsreq() {
    logger::event_log(this, "bad ls req");
    event_pre_aspect
    lsr_manager.clear();
    dd_manager.reset();
    state = EXSTART;
    interface->send_dd_packet(this);
    event_post_aspect
}   

void Neighbor::event_loading_done() {
    logger::event_log(this, "loading done");
    event_pre_aspect
    this->state     = FULL;

    event_post_aspect
}   

void Neighbor::event_is_adj_ok() {
    logger::event_log(this, "adj_ok?");
    event_pre_aspect

    if (this->state == _2WAY) {
        if (this->interface->dr == this->interface->ip
                || this->interface->bdr == this->interface->ip
                || this->interface->dr == this->ip
                || this->interface->bdr == this->ip) {
            dd_manager.reset();
            state = EXSTART;
            interface->send_dd_packet(this);
        } else {
            this->state = _2WAY;
        }
    } else if (this->state >= EXSTART) {
        if (this->interface->dr != this->interface->ip
                && this->interface->bdr != this->interface->ip
                && this->interface->dr != this->ip
                && this->interface->bdr != this->ip) {
            this->state = _2WAY;
            lsr_manager.clear();
            lsu_retransmit_manager.timer.clear();
        }
    }

    event_post_aspect
}   

void Neighbor::event_seq_num_mismatch() {
    logger::event_log(this, "seq num mismatch");
    event_pre_aspect
    lsr_manager.clear();
    dd_manager.reset();
    state = EXSTART;
    interface->send_dd_packet(this);
    event_post_aspect
}  

void Neighbor::event_1way_received() {
    logger::event_log(this, "event 1way");
    event_pre_aspect
    this->state = INIT;
    this->interface->event_neighbor_change();
    event_post_aspect
}   

void Neighbor::event_kill_nbr() {
    logger::event_log(this, "kill neighbor");
    event_pre_aspect
    this->state = N_DOWN;
    this->interface->event_neighbor_change();
    event_post_aspect
}   

void Neighbor::event_inactivity_timer() {
    logger::event_log(this, "loading done");
    event_pre_aspect
    this->state     = N_DOWN;
    this->interface->event_neighbor_change();

    event_post_aspect
}  

void Neighbor::event_ll_down() {
    logger::event_log(this, "ll down");
    event_pre_aspect
    this->state     = N_DOWN;

    this->interface->event_neighbor_change();
    event_post_aspect
}   

int Neighbor::DDManager::fill_lsa_headers(LSAHeader headers[]) {
    int i = 0;
    int max = (router::config::MTU - sizeof(OSPFDD)) / sizeof(LSAHeader);
    for (;this->recorder < (int)dd_r_lsa_headers.size() && i < max; i++, this->recorder++) {
        headers[i] = *dd_r_lsa_headers[i];
        //headers[i].show();
        headers[i].hton();
    }
    return i;
}

bool Neighbor::DDManager::has_more() {
    return recorder < (int) dd_r_lsa_headers.size();
}

void Neighbor::DDManager::remove(LSAHeader *r_lsa, LSAHeader *new_r_lsa) {
    for (uint32_t i = 0; i < dd_r_lsa_headers.size(); i++) {
        //如果要清除的实例比dd列表中的实例相同或者新
        if (dd_r_lsa_headers[i] == r_lsa) {
            if (new_r_lsa == NULL) {
                dd_r_lsa_headers.erase(dd_r_lsa_headers.begin() + i);
                if (recorder >= i) {
                    recorder--;
                }
            } else {
                dd_r_lsa_headers[i] = new_r_lsa;
            }
        }
    }
}

void Neighbor::DDManager::reset() {
    timer = -1;
    seq_number = router::router_id;
    b_MS = true;
    b_M = true;
    b_I = true;
    memset(last_recv, 0, 4096);
    memset(last_send, 0, 4096);
    recorder = 0;
    dd_r_lsa_headers.clear();
}

void Neighbor::DDManager::prepare() {
    router::lsa_db.get_all_lsa(dd_r_lsa_headers);
    recorder = 0;
}

bool Neighbor::LSRManager::rm_lsa(LSAHeader *v_lsa) {
    for (int i = req_v_lsas.size() - 1; i >= 0; i--) {
        if (v_lsa->compare(&req_v_lsas[i]) <= 0) {
            req_v_lsas.erase(req_v_lsas.begin() + i);
            return true;
        }
    }
    return false;
}

void Neighbor::LSRManager::clear() {
    req_v_lsas.clear();
}

void Neighbor::LSRManager::add_lsa(LSAHeader *v_lsa) {
    req_v_lsas.push_back(*v_lsa);
}

void Neighbor::LSUManager::step_one() {
    for (auto it = timer.begin(); it != timer.end(); ++it) {
        it->second--;
    }
}

void Neighbor::LSUManager::get_retransmit_lsas(std::vector<LSAHeader*>& r_lsas) {
    for (auto& pair : timer) {
        if (pair.second <= 0) {
            r_lsas.push_back(pair.first);
            pair.second = 5;
        }
    }
}

void Neighbor::LSUManager::remove_lsa(LSAHeader* r_lsa) {
    for (auto it = timer.begin(); it != timer.end();) {
        //清除与lsa相同或者比lsa更老的实例
        if (it->first->compare(r_lsa) >= 0) {
            timer.erase(it++->first);
            return;
        } else {
            ++it;
        }
    }
}

void Neighbor::LSUManager::add_lsa(LSAHeader* r_lsa) {
    timer[r_lsa] = 5;
}