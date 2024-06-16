#ifndef LSA_H
#define LSA_H

#include <stdint.h>
#include <stddef.h>
#include "../interface/interface.h"
struct LSAHeader {
    uint16_t    ls_age;                // LSA的年龄
    uint8_t     options;               // 支持的特性和功能
    uint8_t     ls_type;               // LSA类型
    uint32_t    link_state_id;         // LSA标识符
    uint32_t    advertising_router;    // 发布该LSA的路由器ID
    uint32_t    ls_sequence_number;    // LSA序列号
    uint16_t    ls_checksum;           // 校验和
    uint16_t    length;                // LSA总长度

    bool operator==(LSAHeader &another);
};

struct RouterLSALink {
    uint32_t    link_id;
    uint32_t    link_data;
    uint8_t     type;
    uint8_t     tos_num;
    uint16_t    metric;
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
};

struct NetworkLSA {
    struct LSAHeader header;
    // 以下是特定于Network-LSA的数据
    uint32_t network_mask;
    uint32_t attached_routers[];
};

// struct SummaryLSA {
//     struct LSAHeader;
//     // 以下是特定于Summary-LSA的数据
//     uint32_t network_mask;
//     uint32_t metric;
// };
// 
// struct ASBRSummaryLSA {
//     struct LSAHeader;
//     // 以下是特定于ASBR-Summary-LSA的数据
//     uint32_t network_mask;
//     uint32_t metric;
// };
// 
// struct ASExternalLSA {
//     struct LSAHeader;
//     // 以下是特定于AS-External-LSA的数据
//     uint32_t network_mask;
//     struct {
//         uint32_t metric : 24;
//         uint32_t e_bit : 1;
//         uint32_t reserved : 7;
//     } metric;
//     uint32_t forwarding_address;
//     uint32_t external_route_tag;
// };


#endif 
