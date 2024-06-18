#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"
#include "../../include/db/lsa_db.h"

#define mark_state NeighborState pre_state = this->state;
#define print_state_log if (pre_state != this->state) {logger::state_transition_log(this, pre_state, this->state);}

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
    this->dd_last_recv = (OSPFDD*)malloc(4096);
    this->dd_last_send = (OSPFDD*)malloc(4096);
}

Neighbor::Neighbor() {
}



void Neighbor::event_hello_received() {
    logger::event_log(this, "hello_received");
    mark_state

    if (this->state == NeighborState::N_DOWN) {
        this->inactivity_timer = this->interface->dead_interval;
        this->state = INIT;
    } else {
        this->inactivity_timer = this->interface->dead_interval;
    }

    print_state_log
}

void Neighbor::event_start() {
    //NBMA网络暂时不管
}   

void Neighbor::event_2way_received(Interface *interface) {
    logger::event_log(this, "2way_received");
    mark_state

    if (this->state != INIT) {
        goto end;
    }
    if (this->interface->dr == this->interface->ip
            || this->interface->bdr == this->interface->ip
            || this->interface->dr == this->ip
            || this->interface->bdr == this->ip) {
        this->dd_sequence_number = router::router_id;
        this->dd_reset_lsas();
        this->state = EXSTART;
        interface->send_dd_packet(this);
    } else {
        this->state = _2WAY;
    }

end:
    print_state_log
}

void Neighbor::event_negotiation_done() {
    logger::event_log(this, "negotiation_done");
    mark_state

    this->state = NeighborState::EXCHANGE;
    print_state_log
}

void Neighbor::event_exchange_done() {
    logger::event_log(this, "exchange done");
    mark_state

    this->rxmt_timer = -1;
    this->state      = this->req_lsas.size() == 0 ? FULL : LOADING;

    print_state_log
}   

void Neighbor::event_bad_lsreq() {

}   

void Neighbor::event_loading_done() {

}   

void Neighbor::event_is_adj_ok() {
    logger::event_log(this, "adj_ok?");
    mark_state

    if (this->state == _2WAY) {
        if (this->interface->dr == this->interface->ip
                || this->interface->bdr == this->interface->ip
                || this->interface->dr == this->ip
                || this->interface->bdr == this->ip) {
            this->dd_sequence_number = router::router_id;
            this->dd_reset_lsas();
            this->state = EXSTART;
            interface->send_dd_packet(this);
        } else {
            this->state = _2WAY;
        }
    }

    print_state_log
}   

void Neighbor::event_seq_num_mismatch() {

}  

void Neighbor::event_1way() {

}   

void Neighbor::event_kill_nbr() {

}   

void Neighbor::event_inactivity_timer() {

}  

void Neighbor::event_ll_down() {

}   

void Neighbor::dd_reset_lsas() {
    this->dd_recorder = 0;
    LSADatabase *db = &router::lsa_db;
    db->get_all_lsa(&this->dd_lsa_headers);
    req_lsas.clear();
}

int Neighbor::fill_lsa_headers(LSAHeader *headers) {
    int i = 0;
    int max = (router::config::MTU - sizeof(OSPFDD)) / sizeof(LSAHeader);
    for (;this->dd_recorder < dd_lsa_headers.size() && i < max; i++, this->dd_recorder++) {
        headers[i] = *dd_lsa_headers[i];
        headers[i].hton();
    }
    return i;
}

bool Neighbor::dd_has_more_lsa() {
    return dd_recorder < dd_lsa_headers.size();
}


