#include "../../include/db/lsa_db.h"

LSADatabase::LSADatabase(Interface *interface) {
    this->interface = interface;
}

void LSADatabase::regenerate_router_lsa() {

}

bool LSADatabase::has_lsa(LSAHeader *header) {
    if (*((LSAHeader*)this->network_lsa) == *header) {
        return true;
    }
    if (*((LSAHeader*)this->my_router_lsa) == *header) {
        return true;
    }
    for (auto router_lsa : this->router_lsas) {
        if (*((LSAHeader*)router_lsa) == *header) {
            return true;
        }
    }
    return false;
}