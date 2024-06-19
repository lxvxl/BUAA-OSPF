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

void Interface::clear_invalid_req(LSAHeader *old_lsa, LSAHeader *new_lsa) {
    for (auto neighbor : neighbors) {
        //清除待请求的lsa
        for (int i = neighbor->req_lsas.size() - 1; i >= 0; i--) {
            LSAHeader::Relation rel = neighbor->req_lsas[i]->compare(old_lsa);
            if (rel == LSAHeader::OLDER || rel == LSAHeader::SAME) {
                neighbor->req_lsas.erase(neighbor->req_lsas.begin() + i);
            }
        }
        //清除待发送的dd包中的相应lsa。如果new_lsa不为空，则使用new_lsa来代替原有的lsa。
        for (int i = neighbor->dd_lsa_headers.size() - 1; i >= 0; i--) {
            LSAHeader::Relation rel = neighbor->req_lsas[i]->compare(old_lsa);
            if (rel == LSAHeader::OLDER || rel == LSAHeader::SAME) {
                if (new_lsa == NULL) {
                    neighbor->dd_lsa_headers.erase(neighbor->req_lsas.begin() + i);
                    neighbor->dd_recorder--;
                } else {
                    neighbor->dd_lsa_headers[i] = new_lsa;
                }
            }
        }
    }
}
