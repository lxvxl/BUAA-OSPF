#ifndef ROUTER_H
#define ROUTER_H
#include <vector>
#include "../interface/interface.h"

struct Interface;
namespace router {
    std::vector<Interface*> interfaces; 
    namespace config {
        const uint8_t options = 2; 
        const uint16_t MTU = 1500;
    }
}


#endif