#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/common.h"
#include "../../include/logger/logger.h"

#define mark_state NeighborState pre_state = this->state;
#define print_state_log logger::state_transition_log(this, pre_state, this->state);

struct NEIGHBOR* generate_from_hello(struct OSPFHello *hello_packet, Interface *interface) {
    struct NEIGHBOR *neighbor = new NEIGHBOR;
    neighbor->router_id = hello_packet->header.router_id;
    neighbor->priority  = hello_packet->rtr_priority;
    neighbor->ip        = hello_packet->header.ipv4_header.saddr;
    neighbor->dr        = hello_packet->designated_router;
    neighbor->bdr       = hello_packet->backup_designated_router;
    neighbor->interface = interface;
    return neighbor;
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

void Neighbor::event_2way_received() {
    logger::event_log(this, "2way_received");
    mark_state

    if (this->state != INIT) {
        goto end;
    }
    if (this->interface->dr == this->interface->ip
            || this->interface->bdr == this->interface->ip
            || this->interface->dr == this->ip
            || this->interface->bdr == this->ip) {
        this->state = EXSTART;
    } else {
        this->state = _2WAY;
    }

end:
    print_state_log
}

void Neighbor::event_negotiation_done() {

}

void Neighbor::event_exchange_done() {

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
            this->state = EXSTART;
        } else {
            this->state = _2WAY;
        }
    }

end:
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

