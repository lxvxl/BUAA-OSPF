#ifndef ROUTER_H
#define ROUTER_H
#include <vector>
#include "../interface/interface.h"
#include <stdint.h>
#include "../db/lsa_db.h"

struct Interface;
namespace router {
    extern std::vector<Interface*>  interfaces; 
    extern uint32_t                 router_id;
    extern LSADatabase              lsa_db;
    namespace config {
        extern const uint8_t  options; 
        extern const uint16_t MTU;
        extern const int      min_ls_arrival;        
    }
}


#endif