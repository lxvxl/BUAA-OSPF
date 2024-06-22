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
    while (true) {
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
    //protected_lsas.erase(old_r_lsa);
    for (auto interface : router::interfaces) {
        interface->clear_invalid_req(old_r_lsa, new_r_lsa);
    }
    delete old_r_lsa;
}

LSAHeader* LSADatabase::update(LSAHeader *v_lsa) {
    LSAHeader *old_r_lsa = get_lsa(v_lsa);
    if (old_r_lsa != NULL && old_r_lsa->compare(v_lsa) <= 0) {
        return old_r_lsa;
    }
    switch(v_lsa->ls_type) {
        case LSType::ROUTER: {
            RouterLSA *new_r_lsa = (RouterLSA*)malloc(v_lsa->length);
            memcpy(new_r_lsa, v_lsa, v_lsa->length);
            router_lsas.push_back(new_r_lsa);
            protected_lsas[(LSAHeader*)new_r_lsa] = 1;
            if (get_lsa(v_lsa) != NULL) {
                for (auto it = router_lsas.begin(); it != router_lsas.end(); it++) {
                    if ((LSAHeader*)*it == old_r_lsa) {
                        router_lsas.erase(it);
                        clear_invalid_lsa(old_r_lsa, (LSAHeader*)new_r_lsa);
                        break;
                    }
                }
            }
            return (LSAHeader*)new_r_lsa;
        }
        case LSType::NETWORK: { 
            NetworkLSA *new_r_lsa = (NetworkLSA*)malloc(v_lsa->length);
            memcpy(new_r_lsa, v_lsa, v_lsa->length);
            network_lsas.push_back(new_r_lsa);
            protected_lsas[(LSAHeader*)new_r_lsa] = 1;
            if (get_lsa(v_lsa) != NULL) {
                for (auto it = network_lsas.begin(); it != network_lsas.end(); it++) {
                    if ((LSAHeader*)*it == old_r_lsa) {
                        network_lsas.erase(it);
                        clear_invalid_lsa(old_r_lsa, (LSAHeader*)new_r_lsa);
                        break;
                    }
                }
            }
            return (LSAHeader*)new_r_lsa;
        }
        default:
            return NULL;
    }
}

void LSADatabase::flood(LSAHeader *r_lsa, Interface* origin, InterfaceState sender_state) {
    //先获取合格接口
    std::vector<Interface*> valid_interfaces;
    for (Interface* interface : router::interfaces) {
        if (interface != origin) {
            valid_interfaces.push_back(interface);
        }
    }

    for (Interface* interface : valid_interfaces) {
        bool lsa_involved = false;
        //检查每一个邻居
        for (Neighbor *neighbor : interface->neighbors) {
            if (neighbor->state < EXCHANGE) {
                continue;
            }
            if (neighbor->state == EXCHANGE || neighbor->state == LOADING) {
                neighbor->rm_from_reqs(r_lsa);
            }
            if (origin == interface) {
                continue;
            }
            neighbor->lsu_retransmit_manager.add_lsa(r_lsa);
            lsa_involved = true;
        }
        if (!lsa_involved) {
            continue;
        }
        if (interface == origin) {
            if (sender_state == DR || sender_state == BACKUP) {
                continue;
            }
            if (interface->state == BACKUP) {
                continue;
            }
        }
        uint32_t d_addr;
        if (interface->state == DR || interface->state == BACKUP) {
            d_addr = inet_addr("224.0.0.5");
        } else {
            d_addr = inet_addr("224.0.0.6");
        }
        interface->send_lsu_packet(r_lsa, d_addr);
    }
}

void LSADatabase::generate_router_lsa() {
    RouterLSA *router_lsa = RouterLSA::generate();
    LSAHeader *pre_lsa = get_lsa((LSAHeader*)router_lsa);
    if (pre_lsa != NULL) {
        clear_invalid_lsa(pre_lsa, (LSAHeader*)router_lsa);
    }
    router_lsas.push_back(router_lsa);
    flood((LSAHeader*)router_lsa, NULL, DROTHER);
}

void LSADatabase::generate_network_lsa(Interface *interface) {
    NetworkLSA *network_lsa = NetworkLSA::generate(interface);
    LSAHeader *pre_lsa = get_lsa((LSAHeader*)network_lsa);
    if (pre_lsa != NULL) {
        clear_invalid_lsa(pre_lsa, (LSAHeader*)network_lsa);
    }
    network_lsas.push_back(network_lsa);
    flood((LSAHeader*)network_lsa, NULL, DROTHER);
}

