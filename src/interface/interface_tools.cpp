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
}

void Interface::clear_invalid_req(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa) {
    for (auto neighbor : neighbors) {
        //清除待请求的lsa
        for (int i = neighbor->req_v_lsas.size() - 1; i >= 0; i--) {
            LSAHeader::Relation rel = neighbor->req_v_lsas[i]->compare(old_r_lsa);
            if (rel == LSAHeader::OLDER || rel == LSAHeader::SAME) {
                delete neighbor->req_v_lsas[i];
                neighbor->req_v_lsas.erase(neighbor->req_v_lsas.begin() + i);
            }
        }
        //清除待发送的dd包中的相应lsa。如果new_lsa不为空，则使用new_lsa来代替原有的lsa。
        for (int i = neighbor->dd_r_lsa_headers.size() - 1; i >= 0; i--) {
            LSAHeader::Relation rel = neighbor->dd_r_lsa_headers[i]->compare(old_r_lsa);
            if (rel == LSAHeader::OLDER || rel == LSAHeader::SAME) {
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
