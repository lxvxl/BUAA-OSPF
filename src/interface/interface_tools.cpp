#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"

Neighbor* Interface::get_neighbor_by_id(uint32_t router_id) {
    for (auto n : neighbors) {
        if (n->router_id == router_id) {
            return n;
        }
    }
    return NULL;
}

Neighbor *Interface::get_neighbor_by_ip(uint32_t ip) {
    for (auto n : neighbors) {
        if (n->ip == ip) {
            return n;
        }
    }
    return NULL;
}

Interface::Interface(const char *name, uint32_t ip, uint32_t mask, uint16_t metric) {
    this->name         = name;
    this->ip           = ip;
    this->network_mask = mask;
    this->metric       = metric;
    memset(send_buffer, 0, sizeof(send_buffer));
    memset(recv_buffer, 0, sizeof(recv_buffer));
    router::interfaces.push_back(this);
}

void Interface::clear_invalid_req(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa) {
    for (auto neighbor : neighbors) {
        //清除待发送的dd包中的相应lsa。如果new_lsa不为空，则使用new_lsa来代替原有的lsa。
        neighbor->dd_manager.remove(old_r_lsa, new_r_lsa);
        //清除待重传LSU中的LSA
        neighbor->lsu_retransmit_manager.remove_lsa(old_r_lsa);
    }
}

