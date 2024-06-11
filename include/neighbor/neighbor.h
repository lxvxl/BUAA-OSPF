#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <stdint.h>
#include <stddef.h>
#include <arpa/inet.h>
#include "../packet/packets.h"

enum NeighborState {
    DOWN,
    ATTEMPT,
    INIT,
    _2WAY,
    EXSTART,
    EXCHANGE,
    LOADING,
    FULL
};

typedef struct NEIGHBOR {
    NeighborState   state = NeighborState::DOWN;
    uint32_t        router_id;
    uint8_t         priority;
    uint32_t        ip;
    bool            is_master;


} Neighbor;

struct NEIGHBOR* generate_from_hello(struct OSPFHello *hello_packet);
#endif // PACKET_MANAGE_H