#ifndef LSA_DB_H
#define LSA_DB_H

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include "../packet/packets.h"
#include "../interface/interface.h"

struct Interface;
struct RouterLSA;
struct NetworkLSA;

struct LSADatabase {
    NetworkLSA              *network_lsa    = NULL;
    RouterLSA               *my_router_lsa;
    std::vector<RouterLSA*> router_lsas;
    uint32_t                seq_num         = 0x80000001;
    Interface               *interface;


    LSADatabase(Interface *interface);
    void    regenerate_router_lsa();
    bool    has_lsa(LSAHeader *header);

};

#endif 