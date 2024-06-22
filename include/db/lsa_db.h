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
    uint32_t                    seq_num         = 0x80000001;
    std::map<LSAHeader*, int>   protected_lsas; //保护泛洪来的lsa不会被立刻替代

    std::thread                 update_thread;

    LSADatabase();
    //判断这个LSA是否存在
    LSAHeader*              get_lsa(LSAHeader *v_lsa);
    //生成LSA的时候，获取下一个序号。
    uint32_t                get_seq_num();
    //将vector清空，并获得所有LSA的引用
    void                    get_all_lsa(std::vector<LSAHeader*>& r_lsas);
    //获取指定的LSA，注意都是主机序
    LSAHeader*              get_lsa(uint8_t ls_type, uint32_t link_state_id, uint32_t advertising_router);
    // 更新实例。如果有旧实例还在dd的待传送列表上或LSU重传列表上，将其更新为新的实例
    LSAHeader*              update(LSAHeader *v_lsa);
    //清除实例，如果旧实例在dd待传送列表上且新实例不为NULL，则使用新实例代替旧实例
    void                    clear_invalid_lsa(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa);

    void                    db_thread_runner();
    void                    generate_router_lsa();
    void                    generate_network_lsa(Interface *interface);
    //需要被泛洪的LSA。
    static void             flood(LSAHeader *r_lsa, Interface* origin, InterfaceState sender_state);
};

#endif 