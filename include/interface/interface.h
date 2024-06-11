#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <thread>
#include <arpa/inet.h>
#include "../neighbor/neighbor.h"
#include <vector>
#include "../packet/packets.h"


typedef struct INTERFACE {
    const char* name;
    std::thread rcv_thread;
    std::thread hello_thread;
    uint8_t     rtr_priority                = 1; 
    uint32_t    ip                          = inet_addr("192.168.64.20");
    uint32_t    network_mask                = inet_addr("255.255.255.0");
    uint16_t    hello_interval              = 10;
    uint32_t    dead_interval               = 40;
    uint32_t    designated_route            = 0; // 这里用的是网络字节序
    uint32_t    backup_designated_router    = 0;     
    uint32_t    router_id                   = inet_addr("2.2.2.2");
    uint32_t    area_id                     = 0;
    std::vector<Neighbor *> neighbors;
    
    Neighbor* get_neighbor(uint32_t router_id);
    void generate_hello_packet(OSPFHello *hello_packet);
} Interface;
#endif 