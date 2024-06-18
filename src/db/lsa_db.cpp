#include "../../include/db/lsa_db.h"

void LSADatabase::get_all_lsa(std::vector<LSAHeader*> *vec) {
    vec->clear();
    if (this->network_lsa != NULL) {
        vec->push_back((LSAHeader*)this->network_lsa);
    }
    if (this->my_router_lsa != NULL) {
        vec->push_back((LSAHeader*)this->my_router_lsa);
    }
    for (auto router_lsa : this->router_lsas) {
        vec->push_back((LSAHeader*)router_lsa);
    }
}


LSADatabase::LSADatabase() {
    this->my_router_lsa = RouterLSA::generate();
}

bool LSADatabase::has_lsa(LSAHeader *another) {
    std::vector<LSAHeader*> headers;
    get_all_lsa(&headers);
    for (auto header : headers) {
        switch (header->compare(another))   {
            case LSAHeader::SAME:
            case LSAHeader::NEWER:
                return true;
            case LSAHeader::NOT_SAME:
            case LSAHeader::OLDER:
                break;
        }
    }
    return false;
}

uint32_t LSADatabase::get_seq_num() {
    return this->seq_num++;
}