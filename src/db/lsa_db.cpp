#include "../../include/db/lsa_db.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"
#include <functional>


void LSADatabase::get_all_lsa(std::vector<LSAHeader*>& r_lsas) {
    r_lsas.clear();
    for (auto router_lsa : this->router_lsas) {
        r_lsas.push_back((LSAHeader*)router_lsa);
    }
    for (auto network_lsa : this->network_lsas) {
        r_lsas.push_back((LSAHeader*)network_lsa);
    }
}


LSADatabase::LSADatabase() {
    this->router_lsas.push_back(RouterLSA::generate());
    this->update_thread = std::thread(std::bind(&LSADatabase::db_thread_runner, this));
    this->update_thread.detach();
}

void LSADatabase::db_thread_runner() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    for (RouterLSA *lsa : router_lsas) {
        lsa->header.ls_age++;
    }
    for (NetworkLSA *lsa : network_lsas) {
        lsa->header.ls_age++;
    }
    for (auto it = protected_lsas.begin(); it != protected_lsas.end();) {
        it->second--;
        if (it->second == 0) {
            protected_lsas.erase(it++);
        } else {
            it++;
        }
    }
}

LSAHeader* LSADatabase::get_lsa(LSAHeader *v_lsa) {
    std::vector<LSAHeader*> lsas;
    get_all_lsa(lsas);
    for (auto header : lsas) {
        if (header->same(v_lsa)) {
            return header;
        }
    }
    return NULL;
} 

uint32_t LSADatabase::get_seq_num() {
    return this->seq_num++;
}

LSAHeader* LSADatabase::get_lsa(uint8_t ls_type, uint32_t link_state_id, uint32_t advertising_router) {
    std::vector<LSAHeader*> lsa_vec;
    get_all_lsa(lsa_vec);
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
void LSADatabase::clear_invalid_lsa(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa) {
    protected_lsas.erase(old_r_lsa);
    for (auto interface : router::interfaces) {
        interface->clear_invalid_req(old_r_lsa, new_r_lsa);
    }
}

void LSADatabase::update(LSAHeader *v_lsa) {
    LSAHeader *old_r_lsa;

    switch(v_lsa->ls_type) {
        case LSType::ROUTER: {
            for (int i = router_lsas.size() - 1; i >= 0; i--) {
                old_r_lsa = (LSAHeader*)router_lsas[i];
                switch (old_r_lsa->compare(v_lsa)) {
                    case LSAHeader::SAME:
                    case LSAHeader::NEWER:
                        return;
                    case LSAHeader::OLDER: {
                        RouterLSA *new_r_lsa = (RouterLSA*)malloc(v_lsa->length);
                        memcpy(new_r_lsa, v_lsa, v_lsa->length);
                        clear_invalid_lsa(old_r_lsa, v_lsa);
                        delete old_r_lsa;
                        router_lsas[i] = new_r_lsa;
                        protected_lsas[(LSAHeader*)new_r_lsa] = 1;
                        return;
                    }
                    case LSAHeader::NOT_SAME:
                        break;
                }
            }    
            RouterLSA *new_r_lsa = (RouterLSA*)malloc(v_lsa->length);
            memcpy(new_r_lsa, v_lsa, v_lsa->length);
            router_lsas.push_back(new_r_lsa);
            protected_lsas[(LSAHeader*)new_r_lsa] = 1;
            break;
        }
        case LSType::NETWORK: { 
            for (int i = network_lsas.size() - 1; i >= 0; i--) {
                old_r_lsa = (LSAHeader*)network_lsas[i];
                switch (old_r_lsa->compare(v_lsa)) {
                    case LSAHeader::SAME:
                    case LSAHeader::NEWER:
                        return;
                    case LSAHeader::OLDER: {
                        clear_invalid_lsa(old_r_lsa, v_lsa);
                        NetworkLSA *new_r_lsa = (NetworkLSA*)malloc(v_lsa->length);
                        memcpy(new_r_lsa, v_lsa, v_lsa->length);
                        delete old_r_lsa;
                        network_lsas[i] = new_r_lsa;
                        protected_lsas[(LSAHeader*)new_r_lsa] = 1;
                        return;
                    }
                    case LSAHeader::NOT_SAME:
                        break;
                }
            }    
            NetworkLSA *new_r_lsa = (NetworkLSA*)malloc(v_lsa->length);
            memcpy(new_r_lsa, v_lsa, v_lsa->length);
            network_lsas.push_back(new_r_lsa);
            protected_lsas[(LSAHeader*)new_r_lsa] = 1;
            break;
        }
        default:
            break;
            //TODO
    }
}

