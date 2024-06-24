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
    this->priority  = hello_packet->rtr_priority;
    this->ip        = ip;
    this->dr        = hello_packet->designated_router;
    this->bdr       = hello_packet->backup_designated_router;
    this->interface = interface;
    this->b_I       = 1;
    this->b_M       = 1;
    this->b_MS      = 1;
    this->dd_last_recv = (OSPFDD*)calloc(1, 4096);
    this->dd_last_send = (OSPFDD*)calloc(1, 4096);
}

Neighbor::Neighbor() {
    this->dd_last_recv = (OSPFDD*)calloc(1, 4);
    this->dd_last_send = (OSPFDD*)calloc(1, 4);
}

Neighbor::~Neighbor() {
    free(dd_last_recv);
    free(dd_last_send);
    for (LSAHeader* lsa : req_v_lsas) {
        delete lsa;
    }
}

bool Neighbor::rm_from_reqs(LSAHeader *v_lsa) {
    for (int i = req_v_lsas.size() - 1; i >= 0; i--) {
        if (v_lsa->compare(req_v_lsas[i]) <= 0) {
            delete req_v_lsas[i];
            req_v_lsas.erase(req_v_lsas.begin() + i);
            return true;
        }
    }
    return false;
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
    logger::event_log(this, "2way_received");
    event_pre_aspect

    if (this->state != INIT) {
        goto end;
    }
    if (this->interface->dr == this->interface->ip
            || this->interface->bdr == this->interface->ip
            || this->interface->dr == this->ip
            || this->interface->bdr == this->ip) {
        this->dd_sequence_number = router::router_id;
        this->b_MS = 1;
        this->b_M = 1;
        this->b_I = 1;
        //this->dd_reset_lsas();
        this->state = EXSTART;
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

    this->dd_recorder = 0;
    LSADatabase *db = &router::lsa_db;
    db->get_all_lsa(this->dd_r_lsa_headers);
    for (auto header : req_v_lsas) {
        delete header;
    }
    req_v_lsas.clear();

    this->state = NeighborState::EXCHANGE;
    event_post_aspect
}

void Neighbor::event_exchange_done() {
    logger::event_log(this, "exchange done");
    event_pre_aspect

    this->dd_retransmit_timer = -1;
    if (this->req_v_lsas.size() == 0) {
        this->state = FULL;
        this->lsr_retransmit_timer = -1;
    } else {
        this->state = LOADING;
        this->lsr_retransmit_timer = 1;
    }

    event_post_aspect
}   

void Neighbor::event_bad_lsreq() {
    logger::event_log(this, "bad ls req");
    event_pre_aspect
    for (auto header : req_v_lsas) {
        delete header;
    }
    req_v_lsas.clear();
    this->dd_sequence_number = router::router_id;
    this->b_MS = 1;
    this->b_M = 1;
    this->b_I = 1;
    this->state = EXSTART;
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
            this->dd_sequence_number = router::router_id;
            this->b_MS = 1;
            this->b_M = 1;
            this->b_I = 1;
            //this->dd_reset_lsas();
            this->state = EXSTART;
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
            for (auto header : req_v_lsas) {
                delete header;
            }
            req_v_lsas.clear();
            lsu_retransmit_manager.timer.clear();
        }
    }

    event_post_aspect
}   

void Neighbor::event_seq_num_mismatch() {
    logger::event_log(this, "seq num mismatch");
    event_pre_aspect
    for (auto header : req_v_lsas) {
        delete header;
    }
    req_v_lsas.clear();
    this->dd_sequence_number = router::router_id;
    this->b_MS = 1;
    this->b_M = 1;
    this->b_I = 1;
    this->state = EXSTART;
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

int Neighbor::fill_lsa_headers(LSAHeader *headers) {
    int i = 0;
    int max = (router::config::MTU - sizeof(OSPFDD)) / sizeof(LSAHeader);
    for (;this->dd_recorder < dd_r_lsa_headers.size() && i < max; i++, this->dd_recorder++) {
        headers[i] = *dd_r_lsa_headers[i];
        //headers[i].show();
        headers[i].hton();
    }
    return i;
}

bool Neighbor::dd_has_more_lsa() {
    return dd_recorder < dd_r_lsa_headers.size();
}

void Neighbor::LSURetransmitManager::step_one() {
    for (auto it = timer.begin(); it != timer.end(); ++it) {
        it->second--;
    }
}

void Neighbor::LSURetransmitManager::get_retransmit_lsas(std::vector<LSAHeader*>& r_lsas) {
    for (auto& pair : timer) {
        if (pair.second <= 0) {
            r_lsas.push_back(pair.first);
            pair.second = 5;
        }
    }
}

void Neighbor::LSURetransmitManager::remove_lsa(LSAHeader* lsa) {
    for (auto it = timer.begin(); it != timer.end();) {
        //清除与lsa相同或者比lsa更老的实例
        if (it->first->compare(lsa) >= 0) {
            timer.erase(it++);
        } else {
            it++;
        }
    }
}

void Neighbor::LSURetransmitManager::add_lsa(LSAHeader* r_lsa) {
    timer[r_lsa] = 5;
}