#include "../../include/db/lsa_db.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"


void LSADatabase::get_all_lsa(std::vector<LSAHeader*> *vec) {
    vec->clear();
    for (auto router_lsa : this->router_lsas) {
        vec->push_back((LSAHeader*)router_lsa);
    }
    for (auto network_lsa : this->network_lsas) {
        vec->push_back((LSAHeader*)network_lsa);
    }
}


LSADatabase::LSADatabase() {
    this->router_lsas.push_back(RouterLSA::generate());
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

LSAHeader* LSADatabase::get_lsa(uint8_t ls_type, uint32_t link_state_id, uint32_t advertising_router) {
    std::vector<LSAHeader*> lsa_vec;
    get_all_lsa(&lsa_vec);
    for (auto lsa : lsa_vec) {
        if (lsa->ls_type == ls_type 
                && lsa->link_state_id == link_state_id 
                && lsa->advertising_router == advertising_router) {
            return lsa;
        }
    }
    return NULL;
}

//清除失效的lsa。如果new_lsa不为NULL，则使用new_lsa替换old_lsa
void clear_invalid_lsa(LSAHeader *old_lsa, LSAHeader *new_lsa) {
    for (auto interface : router::interfaces) {
        interface->clear_invalid_req(old_lsa, new_lsa);
    }
}

void LSADatabase::update(LSAHeader *lsa) {
    LSAHeader *header;

    switch(lsa->ls_type) {
        case LSType::ROUTER:
            for (int i = router_lsas.size() - 1; i >= 0; i--) {
                header = (LSAHeader*)router_lsas[i];
                switch (header->compare(lsa)) {
                    case LSAHeader::SAME:
                    case LSAHeader::NEWER:
                        return;
                    case LSAHeader::OLDER:
                        clear_invalid_lsa(header, lsa);
                        delete header;
                        router_lsas.erase(router_lsas.begin() + i);
                        RouterLSA *new_lsa = (RouterLSA*)malloc(lsa->length);
                        memcpy(new_lsa, lsa, lsa->length);
                        router_lsas.push_back(new_lsa);
                        return;
                    case LSAHeader::NOT_SAME:
                        break;
                }
            }    
            RouterLSA *new_router_lsa = (RouterLSA*)malloc(lsa->length);
            memcpy(new_router_lsa, lsa, lsa->length);
            router_lsas.push_back(new_router_lsa);
            break;
        case LSType::NETWORK:
            for (int i = network_lsas.size() - 1; i >= 0; i--) {
                header = (LSAHeader*)network_lsas[i];
                switch (header->compare(lsa)) {
                    case LSAHeader::SAME:
                    case LSAHeader::NEWER:
                        return;
                    case LSAHeader::OLDER:
                        clear_invalid_lsa(header, lsa);
                        delete header;
                        network_lsas.erase(network_lsas.begin() + i);
                        NetworkLSA *new_lsa = (NetworkLSA*)malloc(lsa->length);
                        memcpy(new_lsa, lsa, lsa->length);
                        network_lsas.push_back(new_lsa);
                        return;
                    case LSAHeader::NOT_SAME:
                        break;
                }
            }    
            NetworkLSA *new_network_lsa = (NetworkLSA*)malloc(lsa->length);
            memcpy(new_network_lsa, lsa, lsa->length);
            network_lsas.push_back(new_network_lsa);
            break;
        default:
            //TODO
    }
}

