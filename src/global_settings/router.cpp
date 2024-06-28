#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"
#include <stdexcept>
#include <iostream>

std::vector<Interface*> router::interfaces;
uint32_t router::router_id = inet_addr("2.2.2.2");
std::unordered_map<uint32_t, LSADatabase> router::area_lsa_dbs; // 初始化区域LSADatabase容器
std::mutex router::mutex;

const uint8_t router::config::options = 0x42;
const uint16_t router::config::MTU = 1500;
const int router::config::min_ls_arrival = 1;


void router::register_area(uint32_t area_id) {
    std::lock_guard<std::mutex> lock(mutex);
    // 如果区域ID不存在，则实例化新的LSADatabase
    if (area_lsa_dbs.find(area_id) == area_lsa_dbs.end()) {
        area_lsa_dbs.emplace(area_id, area_id);
        std::cout << "Registered new area with ID: " << area_id << std::endl;
    } else {
        std::cout << "Area with ID: " << area_id << " already exists." << std::endl;
    }
}

bool router::is_abr() {
    return area_lsa_dbs.size() > 1;
}


bool router::is_asbr() {
    return false;
}