#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"

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

Interface::Interface(const char *name)
{
    this->name = name;
    memset(send_buffer, 0, sizeof(send_buffer));
    memset(recv_buffer, 0, sizeof(recv_buffer));
}

void Interface::clear_invalid_req(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa) {
    for (auto neighbor : neighbors) {
        //清除待发送的dd包中的相应lsa。如果new_lsa不为空，则使用new_lsa来代替原有的lsa。
        for (int i = neighbor->dd_r_lsa_headers.size() - 1; i >= 0; i--) {
            //如果要清除的实例比dd列表中的实例相同或者新
            if (old_r_lsa->compare(neighbor->dd_r_lsa_headers[i]) <= 0) {
                if (new_r_lsa == NULL) {
                    neighbor->dd_r_lsa_headers.erase(neighbor->dd_r_lsa_headers.begin() + i);
                    if (neighbor->dd_recorder >= i) {
                        neighbor->dd_recorder--;
                    }
                } else {
                    neighbor->dd_r_lsa_headers[i] = new_r_lsa;
                }
            }
        }
        //清除待重传LSU中的LSA
        neighbor->lsu_retransmit_manager.remove_lsa(old_r_lsa);
    }
}
