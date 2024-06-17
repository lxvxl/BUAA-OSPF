#ifndef ROUTER_H
#define ROUTER_H
#include <vector>
#include "../interface/interface.h"
#include <stdint.h>

struct Interface;
namespace router {
    extern std::vector<Interface*> interfaces; 
    namespace config {
        extern const uint8_t options; 
        extern const uint16_t MTU;
    }
}


#endif