#include "../../include/neighbor/neighbor.h"
#include "../../include/global_settings/common.h"

struct NEIGHBOR* generate_from_hello(struct OSPFHello *hello_packet) {
    struct NEIGHBOR *neighbor = new NEIGHBOR;
    neighbor->router_id = hello_packet->header.router_id;
    neighbor->priority  = hello_packet->rtr_priority;
    neighbor->ip        = hello_packet->header.ipv4_header.saddr;
    return neighbor;
}