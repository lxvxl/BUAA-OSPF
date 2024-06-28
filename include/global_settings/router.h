#ifndef ROUTER_H
#define ROUTER_H
#include <vector>
#include <unordered_map>
#include "../interface/interface.h"
#include <stdint.h>
#include "../db/lsa_db.h"
#include "../routing/routing.h"
#include <mutex>

struct Interface;
namespace router {
    extern std::vector<Interface*> interfaces; 
    extern uint32_t router_id;
    extern std::unordered_map<uint32_t, LSADatabase> area_lsa_dbs; // 区域ID到LSADatabase实例的映射
    extern std::mutex mutex;

    namespace config {
        extern const uint8_t options; 
        extern const uint16_t MTU;
        extern const int min_ls_arrival;        
    }

    // 注册区域函数
    void register_area(uint32_t area_id);
    bool is_abr();
    bool is_asbr();
}

#endif // ROUTER_H
