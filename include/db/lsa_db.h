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
struct LSAHeader;

struct LSADatabase {
    NetworkLSA              *network_lsa    = NULL;
    RouterLSA               *my_router_lsa;
    std::vector<RouterLSA*> router_lsas;
    uint32_t                seq_num         = 0x80000001;

    LSADatabase();
    //判断这个LSA是否存在
    bool                    has_lsa(LSAHeader *header);
    //生成LSA的时候，获取下一个序号。
    uint32_t                get_seq_num();
    //将vector清空，并获得所有LSA的引用
    void                    get_all_lsa(std::vector<LSAHeader*> *vec);
};

#endif 