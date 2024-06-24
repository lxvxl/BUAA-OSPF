#ifndef ROUTER_H
#define ROUTER_H
#include <vector>
#include "../interface/interface.h"
#include <stdint.h>
#include "../db/lsa_db.h"
#include "../routing/routing.h"
#include <mutex>

struct Interface;
namespace router {
    extern std::vector<Interface*>  interfaces; 
    extern uint32_t                 router_id;
    extern LSADatabase              lsa_db;
    extern std::mutex               mutex;
    extern RoutingTable             routing_table;

    namespace config {
        extern const uint8_t  options; 
        extern const uint16_t MTU;
        extern const int      min_ls_arrival;        
    }
}


#endif