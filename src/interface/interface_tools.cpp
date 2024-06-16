#include "../../include/packet/packets.h"
#include "../../include/interface/interface.h"
#include "../../include/global_settings/common.h"

Neighbor* Interface::get_neighbor(uint32_t router_id) {
    for (auto n : neighbors) {
        if (n->router_id == router_id) {
            return n;
        }
    }
    return NULL;
}

Interface::Interface(const char *name) {
    this->name = name;
    this->db   = new LSADatabase(this); 
}