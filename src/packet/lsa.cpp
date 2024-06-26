#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"
#include "../../include/global_settings/router.h"
#include "../../include/logger/logger.h"
//LSAHeader::Relation LSAHeader::compare(LSAHeader *another) {
//    //这里有问题！！没有完全实现
//    if (ls_type != another->ls_type 
//            || link_state_id != another->link_state_id 
//            || advertising_router != another->advertising_router) {
//        return NOT_SAME;
//    }
//
//    if (ls_seq_num > another->ls_seq_num) {
//        return NEWER;
//    } else if (ls_seq_num < another->ls_seq_num) {
//        return OLDER;
//    }
//
//    // Checksums are the same, compare ages
//    if (ls_age > another->ls_age) {
//        return OLDER;
//    } else if (ls_age < another->ls_age) {
//        return NEWER;
//    }
//
//    return SAME;
//}

bool LSAHeader::same(LSAHeader *another) {
    if (ls_type != another->ls_type 
            || link_state_id != another->link_state_id 
            || advertising_router != another->advertising_router) {
        return false;
    }
    return true;
}

int LSAHeader::compare(LSAHeader *another) {
    if (ls_seq_num > another->ls_seq_num) {
        return -1;
    } else if (ls_seq_num < another->ls_seq_num) {
        return 1;
    }

    if (ls_age > another->ls_age) {
        return 1;
    } else if (ls_age < another->ls_age) {
        return -1;
    }
    return 0;
}

void LSAHeader::cal_checksum() {
    const uint8_t* ptr = ((uint8_t*)this) + 2;
    int length = ntohs(this->length) - 2;

    int32_t x, y;
	uint32_t mul;
	uint32_t c0 = 0, c1 = 0;
	uint16_t checksum = 0;
    const int checksum_offset = 14;

	for (int index = 0; index < length; index++) {
		if (index == checksum_offset ||
			index == checksum_offset+1) {
            // in case checksum has not set 0 before
			c1 += c0;
			ptr++;
		} else {
			c0 = c0 + *(ptr++);
			c1 += c0;
		}
	}

	c0 = c0 % 255;
	c1 = c1 % 255;	
    mul = (length - checksum_offset)*(c0);
  
	x = mul - c0 - c1;
	y = c1 - mul - 1;

	if ( y >= 0 ) y++;
	if ( x < 0 ) x--;

	x %= 255;
	y %= 255;

	if (x == 0) x = 255;
	if (y == 0) y = 255;

	y &= 0x00FF;
    this->ls_checksum = htons((x << 8) | y);
  
	//return (x << 8) | y;
}

void LSAHeader::fill(LSType type, uint32_t link_state_id, uint32_t ls_seq_num, uint16_t length)
{
    this->ls_age              = 1;                
    this->options             = router::config::options;               
    this->ls_type             = type;             
    this->advertising_router  = router::router_id;    
    this->ls_seq_num          = ls_seq_num;    
    this->length              = length;     
    this->link_state_id       = link_state_id;          
    this->ls_checksum         = 0;        

    uint8_t* data = (uint8_t*)this;
    data += 2;

    uint32_t c0 = 0;
    uint32_t c1 = 0;

    for (int i = 0; i < length - 2; ++i) {
        c0 = (c0 + data[i]) % 255;
        c1 = (c1 + c0) % 255;
    }
    this->ls_checksum = (uint16_t)((c1 << 8) | c0);
}

void LSAHeader::show() {
    std::cout<<"ls_age:\t"<<this->ls_age<<std::endl;
    std::cout<<"ls_type:\t"<<this->ls_type<<std::endl;
    std::cout<<"ad_router:\t"<<inet_ntoa({this->advertising_router})<<std::endl;
    std::cout<<"seq_num:\t"<<std::hex<<this->ls_seq_num<<std::endl;
    std::cout<<"length:\t"<<this->length<<std::endl;
    std::cout<<"link_state_id:\t"<<inet_ntoa({link_state_id})<<std::endl;
}


void LSAHeader::hton() {
    this->ls_age      = htons(this->ls_age);
    this->ls_seq_num  = htonl(this->ls_seq_num);
    this->length      = htons(this->length);
    this->ls_checksum = htons(this->ls_checksum);
}


void LSAHeader::ntoh() {
    this->ls_age      = ntohs(this->ls_age);
    this->ls_seq_num  = ntohl(this->ls_seq_num);
    this->length      = ntohs(this->length);
    this->ls_checksum = ntohs(this->ls_checksum);
}

RouterLSALink::RouterLSALink(uint32_t link_id, uint32_t link_data, uint8_t type, uint8_t tos_num, uint16_t metric) {
    this->link_id = link_id;
    this->link_data = link_data;
    this->type = type;
    this->tos_num = tos_num;
    this->metric = metric;
}

RouterLSA *RouterLSA::generate() {
    std::vector<RouterLSALink> links;
    for (auto interface : router::interfaces) {
        if (interface->state == DOWN) {
            continue;
        }
        // 已经与DR建立关系
        if (interface->state != WAITING) {
            //如果路由器自身为DR且与至少一台其他路由器邻接，加入类型 2 连接（传输网络）。否则，加入类型3
            if (interface->dr == interface->ip){
                if (interface->neighbors.size() > 0) {
                    links.emplace_back(RouterLSALink(interface->dr, 
                                                interface->ip, 
                                                (uint8_t)TRAN, 
                                                (uint8_t)0, 
                                                interface->metric));
                } else {
                    links.emplace_back(RouterLSALink(interface->ip, 
                                                interface->network_mask, 
                                                (uint8_t)STUB, 
                                                (uint8_t)0, 
                                                interface->metric));
                }
                continue;
            }
            //如果路由器与 DR 完全邻接，加入类型3连接
            if (interface->get_neighbor_by_ip(interface->dr)->state == FULL) {
                links.emplace_back(RouterLSALink(interface->dr, 
                                                interface->ip, 
                                                (uint8_t)TRAN, 
                                                (uint8_t)0, 
                                                interface->metric));
            } else {
                links.emplace_back(RouterLSALink(interface->ip, 
                                                interface->network_mask, 
                                                (uint8_t)STUB, 
                                                (uint8_t)0, 
                                                interface->metric));
            }
        }

        if (interface->state == WAITING) {
            links.emplace_back(RouterLSALink(interface->ip, 
                                            interface->network_mask, 
                                            (uint8_t)STUB, 
                                            (uint8_t)0, 
                                            interface->metric));
        }
    }
    uint16_t length = sizeof(RouterLSA) + links.size() * sizeof(RouterLSALink);
    RouterLSA *router_lsa = (RouterLSA*)malloc(length);
    memset(router_lsa, 0, length);
    router_lsa->link_num = links.size();
    for (int i = 0; i < links.size(); i++) {
        router_lsa->links[i] = links[i];
    }
    ((LSAHeader*)router_lsa)->fill(ROUTER, router::router_id, router::lsa_db.get_seq_num(), length);
    return router_lsa;
}

void RouterLSA::hton() {
    for (int i = 0; i < this->link_num; i++) {
        this->links[i].metric = htons(this->links[i].metric);
    }
    this->link_num= htons(this->link_num);
    this->header.hton();
}

void RouterLSA::ntoh() {
    this->link_num = ntohs(this->link_num);
    for (int i = 0; i < this->link_num; i++) {
        this->links[i].metric = ntohs(this->links[i].metric);
    }
    this->header.ntoh();
}

void RouterLSA::show() {
    std::cout<<std::endl<<"=====RouterLSA====="<<std::endl;
    this->header.show();
    for (int i = 0; i < get_link_num(); i++) {
        std::cout<<"\tlink id: "<<inet_ntoa({links[i].link_id})<<std::endl;
        std::cout<<"\tlink_data: "<<inet_ntoa({links[i].link_data})<<std::endl;
        printf("\ttype: %d\n", links[i].type);
        printf("\tmetric: %hd\n", links[i].metric);
        //std::cout<<"\ttype: "<<links[i].type<<std::endl;
        //std::cout<<"\tmetric: "<<links[i].type<<std::endl;
    }
    std::cout<<std::endl;
}

int RouterLSA::get_link_num() {
    return (header.length - sizeof(RouterLSA)) / sizeof(RouterLSALink);
}



void NetworkLSA::hton() {
    this->header.hton();
}

void NetworkLSA::ntoh() {
    this->header.ntoh();
}

NetworkLSA* NetworkLSA::generate(Interface *interface) {
    logger::other_log(interface, "try to generate a Network LSA");
    std::vector<uint32_t> attached_routers;
    attached_routers.push_back(router::router_id);
    for (Neighbor *neighbor : interface->neighbors) {
        attached_routers.push_back(neighbor->router_id);
    }
    uint16_t length = sizeof(NetworkLSA) + attached_routers.size() * sizeof(uint32_t);
    NetworkLSA *network_lsa = (NetworkLSA*)malloc(length);
    memset(network_lsa, 0, length);
    network_lsa->network_mask = interface->network_mask;

    for (int i = 0; i < attached_routers.size(); i++) {
        network_lsa->attached_routers[i] = attached_routers[i];
    }
    ((LSAHeader*)network_lsa)->fill(NETWORK, interface->ip, router::lsa_db.get_seq_num(), length);
    return network_lsa;
}

void NetworkLSA::show() {
    std::cout<<std::endl<<"=====NetworkLSA====="<<std::endl;
    this->header.show();
    std::cout<<"mask: "<<inet_ntoa({network_mask})<<std::endl;
    for (int i = 0; i < get_routers_num(); i++) {
        std::cout<<"\tlink id: "<<inet_ntoa({attached_routers[i]})<<std::endl;
    }
    std::cout<<std::endl;
}

int NetworkLSA::get_routers_num() {
    return (header.length - sizeof(NetworkLSA)) / sizeof(uint32_t);
}