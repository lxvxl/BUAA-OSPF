#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/ip.h>

enum OSPFPacketType {
    HELLO = 1,
    DD,
    LSR,
    LSU,
    LSA,
    ERROR
};

struct OSPFHeader {
    struct iphdr ipv4_header;
    uint8_t version;           // 版本号
    uint8_t type;              // 报文类型
    uint16_t packet_length;     // 报文长度
    uint32_t router_id;         // 路由器ID
    uint32_t area_id;           // 区域ID
    uint16_t checksum;         // 校验和
    uint16_t authType;         // 认证类型
    uint32_t authentication[2];
    void show();
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
    void show();
    int get_neighbor_num();
    bool has_neighbor(uint32_t router_id);
};

struct OSPFDD {
    struct OSPFHeader header;
    uint16_t interface_mtu;   // 接口MTU
    uint8_t options;
    uint8_t     b_MS: 1;
    uint8_t     b_M : 1;
    uint8_t     b_I : 1;
    uint8_t     b_other: 5;
    uint32_t dd_sequence_number; // DD序列号
    // 此处可以包含LSA头的列表
    void show();
};

struct OSPFLSR {
    struct OSPFHeader header;
    uint32_t ls_type;       // 链路状态类型
    uint32_t link_state_id; // 链路状态ID
    uint32_t advertising_router; // 广告路由器
    void show();
};

struct OSPFLSU {
    struct OSPFHeader header;
    uint32_t num_lsas; // LSA数量
    // 此处可以包含LSA的列表
    void show();
};

struct OSPFLSA {
    struct OSPFHeader header;
    uint32_t ls_type;       // 链路状态类型
    uint32_t link_state_id; // 链路状态ID
    uint32_t advertising_router; // 广告路由器
    uint32_t ls_sequence_number; // 链路状态序列号
    uint16_t ls_age;       // 链路状态年龄
    uint16_t checksum;     // 校验和
    // 其他可能的LSA字段
    void show();
};



void show_ipv4_header(struct iphdr *header);

#endif // PACKET_MANAGE_H