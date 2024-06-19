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
    std::vector<RouterLSA*>     router_lsas;
    std::vector<NetworkLSA*>    network_lsas;
    uint32_t                seq_num         = 0x80000001;

    LSADatabase();
    //判断这个LSA是否存在
    bool                    has_lsa(LSAHeader *header);
    //生成LSA的时候，获取下一个序号。
    uint32_t                get_seq_num();
    //将vector清空，并获得所有LSA的引用
    void                    get_all_lsa(std::vector<LSAHeader*> *vec);
    //获取指定的LSA，注意都是主机序
    LSAHeader*              get_lsa(uint8_t ls_type, uint32_t link_state_id, uint32_t advertising_router);
    void                    update(LSAHeader *lsa);
};

#endif 