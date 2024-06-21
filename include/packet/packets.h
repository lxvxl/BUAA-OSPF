#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/ip.h>
#include "../interface/interface.h"
#include <vector>

enum OSPFPacketType {
    HELLO = 1,
    DD,
    LSR,
    LSU,
    LSA,
    ERROR
};

enum LSType {
    ROUTER = 1,
    NETWORK,
};

enum LSALinkType {
    P2P = 1,
    TRAN,
    STUB,
    VIRTUAL,
};

struct Interface;
struct Neighbor;
struct LSAHeader;

struct LSAHeader {
    uint16_t    ls_age;                // LSA的年龄
    uint8_t     options;               // 支持的特性和功能
    uint8_t     ls_type;               // LSA类型
    uint32_t    link_state_id;         // LSA标识符
    uint32_t    advertising_router;    // 发布该LSA的路由器ID
    uint32_t    ls_seq_num;            // LSA序列号
    uint16_t    ls_checksum;           // 校验和
    uint16_t    length;                // LSA总长度

    enum Relation {
        SAME,
        NOT_SAME,
        NEWER,
        OLDER,
    };
    //Relation compare(LSAHeader *another);

    void fill(LSType type, uint32_t link_state_id, uint32_t ls_seq_num, uint16_t length);
    void hton();
    void ntoh();
    bool same(LSAHeader *another);
    //返回-1：更新。返回1：更旧
    int compare(LSAHeader *another);
};

struct RouterLSALink {
    uint32_t    link_id;
    uint32_t    link_data;
    uint8_t     type;
    uint8_t     tos_num;
    uint16_t    metric;
    RouterLSALink(uint32_t link_id, uint32_t link_data, uint8_t type, uint8_t tos_num, uint16_t metric);
};

struct RouterLSA {
    struct LSAHeader header;
    // 以下是特定于Router-LSA的数据
    uint8_t     padding_l : 5;
    uint8_t     b_V : 1;    // Virtual : is virtual channel
    uint8_t     b_E : 1;    // External: is ASBR
    uint8_t     b_B : 1;    // Board   : is ABR
    uint8_t     padding_r = 0;
    uint16_t    link_num;
    RouterLSALink links[];

    void hton();
    void ntoh();

    static RouterLSA* generate();
};

struct NetworkLSA {
    struct LSAHeader header;
    // 以下是特定于Network-LSA的数据
    uint32_t network_mask;
    uint32_t attached_routers[];
    void hton();
    void ntoh();
};



struct OSPFHeader {
    uint8_t     version;           // 版本号
    uint8_t     type;              // 报文类型
    uint16_t    packet_length;     // 报文长度
    uint32_t    router_id;         // 路由器ID
    uint32_t    area_id;           // 区域ID
    uint16_t    checksum;         // 校验和
    uint16_t    authType;         // 认证类型
    uint32_t    authentication[2];
    void        show();
    void        fill(OSPFPacketType type, uint32_t area_id, uint16_t packet_length);
};

struct OSPFHello {
    struct OSPFHeader header;
    uint32_t    network_mask;
    uint16_t    hello_interval;
    uint8_t     options;
    uint8_t     rtr_priority;            
    uint32_t    dead_interval;            // 收到邻居回复前的最大等待时间
    uint32_t    designated_router;        // DR
    uint32_t    backup_designated_router; // BDR
    uint32_t    neighbors[];

    void        show();
    int         get_neighbor_num();
    bool        has_neighbor(uint32_t router_id);
    void        fill(Interface *interface);
};

struct OSPFDD {
    struct OSPFHeader header;
    uint16_t    interface_mtu;   // 接口MTU
    uint8_t     options;
    uint8_t     b_MS: 1;
    uint8_t     b_M : 1;
    uint8_t     b_I : 1;
    uint8_t     b_other: 5;
    uint32_t    dd_sequence_number; // DD序列号
    LSAHeader   lsa_headers[];
    // 此处可以包含LSA头的列表

    void        show();
    int         get_lsa_num();
    void        fill(Neighbor *neighbor);
};

struct OSPFLSR {
    struct ReqItem {
        uint32_t ls_type;       // 链路状态类型
        uint32_t link_state_id; // 链路状态ID
        uint32_t advertising_router; // 广告路由器
    };
    OSPFHeader  header; 
    ReqItem     reqs[];
    void        fill(std::vector<LSAHeader*>& headers, Interface *interface);
    int         get_req_num();
};

struct OSPFLSU {
    struct OSPFHeader header;
    uint32_t lsa_num; // LSA数量
    // 此处可以包含LSA的列表
    void show();
    void fill(std::vector<LSAHeader*>& r_lsas, Interface *interface);
};

struct OSPFLSAck {
    struct OSPFHeader header;
    LSAHeader       lsa_headers[];
    void fill(std::vector<LSAHeader*>& v_lsas, Interface *interface);
    int             get_lsa_num();
};



void show_ipv4_header(struct iphdr *header);

#endif