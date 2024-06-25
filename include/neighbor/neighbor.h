#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#include <stdint.h>
#include <stddef.h>
#include <arpa/inet.h>
#include "../packet/packets.h"
#include "../interface/interface.h"
#include <vector>
#include <map>

struct Interface;
struct OSPFDD;
struct OSPFHello;
struct LSAHeader;

enum NeighborState {
    N_DOWN,           //初始状态
    ATTEMPT,        //仅对NBMA网络上的邻居有效，弃用！！ 
    INIT,           //收到了由邻居发送的 Hello 包，但还没有建立与邻居的双向通讯
    _2WAY,          //达成双向通讯
    EXSTART,        //建立邻接的第一步，确定主从
    EXCHANGE,       //交换链路数据库状态中
    LOADING,        //发送LSR包来取得较新的LSA
    FULL            //达到完全邻接
};

struct Neighbor {
    NeighborState   state               = NeighborState::N_DOWN;
    uint32_t        router_id;
    uint32_t        dr;
    uint32_t        bdr;
    uint8_t         priority;
    uint32_t        ip;
    uint32_t        inactivity_timer        = -1;
    uint32_t        dd_retransmit_timer     = -1;
    uint32_t        lsr_retransmit_timer    = -1;

    uint32_t        dd_sequence_number; // DD序列号


    uint8_t         b_MS: 1;
    uint8_t         b_M : 1;
    uint8_t         b_I : 1;
    uint8_t         b_other: 5;

    Interface*      interface;

    OSPFDD*         dd_last_recv;
    OSPFDD*         dd_last_send;
    size_t          dd_recorder;            //dd交换期间，下一个将要发送的header位置
    std::vector<LSAHeader*> dd_r_lsa_headers; //dd交换期间，将要发送的lsa header列表

    std::vector<LSAHeader*> req_v_lsas;       //需要请求的LSA。这个要单独管理内存！


    struct LSURetransmitManager {
        std::map<LSAHeader*, int> timer;        //r_lsa -> time
        void step_one();
        void get_retransmit_lsas(std::vector<LSAHeader*>& r_lsas);
        void remove_lsa(LSAHeader* v_lsa);
        void add_lsa(LSAHeader* r_lsa);
    } lsu_retransmit_manager;

    Neighbor(OSPFHello *hello_packet, Interface *interface, uint32_t ip);
    Neighbor();
    ~Neighbor();

    //从邻居接收一个Hello包
    void            event_hello_received();
    //将以 HelloInterval 秒的间隔向邻居发送 Hello 包     
    void            event_start();
    //在邻居的 Hello 包中包含了路由器自身 
    void            event_2way_received(); 
    //已经协商好主从关系，并交换了 DD 序号
    void            event_negotiation_done();  
    //已成功交换了完整的 DD 包
    void            event_exchange_done();      
    //接收到的连接状态请求中，包含有并不存在于数据库中的 LSA
    void            event_bad_lsreq();          
    //已经接收了数据库中所有需要更新的部分
    void            event_loading_done();    
    //决定是否需要与邻居建立/维持邻接关系
    void            event_is_adj_ok();       
    //接收到的 DD 包出现下列情况：a）含有意外的 DD 序号；b）意外地设定了 Init 位；c）与上一个接收到的 DD 包有着不同的选项域。
    void            event_seq_num_mismatch();
    //从邻居接收到 Hello 包，但并不包含路由器自身。这说明与该邻居的通讯不再是双向。
    void            event_1way_received();           
    //这说明现在不可能与该邻居有任何通讯，强制转换邻居状态到 Down
    void            event_kill_nbr();       
    //非活跃记时器被激活。这说明最近没有从邻居接收到 Hello 包。强制转换邻居状态到 Down。
    void            event_inactivity_timer();
    //由下层协议说明，邻居不可到达。
    void            event_ll_down();     

    bool            dd_has_more_lsa();
    int             fill_lsa_headers(LSAHeader *addr);
    bool            rm_from_reqs(LSAHeader *v_lsa);
};
#endif 