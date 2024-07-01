#ifndef ROUTING_H
#define ROUTING_H

#include <vector>
#include <map>
#include "../packet/packets.h"
#include <stdint.h>
#include <set>
#include <unordered_map>
#include <set>


class Node {
    public:
        bool        is_router;              //描述是否是路由器，否则是一个网络
        uint32_t    id;
        std::unordered_map<Node*, uint16_t> neighbor_nodes; //描述了邻居-权重之间的关系
        Node(bool is_router, uint32_t id) : is_router(is_router), id(id) {};
        virtual void show() = 0;
        virtual ~Node() = default;
};

class RouterNode : public Node {
    public:
        std::unordered_map<Node*, uint32_t> neighbor_2_interface;
        RouterNode(uint32_t id) : Node(true, id) {};
        void show() override;
        ~RouterNode() override = default;
};

class NetNode : public Node {
    public:
        uint32_t    mask;
        NetNode(uint32_t ip, uint32_t mask) : Node(false, ip), mask(mask) {};
        void show() override;
        ~NetNode() override = default;
};



class RoutingTable {
    private:
        std::unordered_map<uint32_t, RouterNode*> router_node_map;
        std::unordered_map<uint32_t, NetNode*> net_node_map;
        RouterNode *me;
        std::unordered_map<Node*, uint32_t> next_step;

        RouterNode* get_router_node(uint32_t id);
        NetNode* get_net_node(uint32_t ip, uint32_t mask);
        
        std::set<std::string> written_routes;
        void dijkstra();
        void reset();
        void add_route(const std::string& target_net, const std::string& target_mask, const std::string& next_net);
    public:
        void generate(std::vector<RouterLSA*>& router_lsas, std::vector<NetworkLSA*>& network_lsas);
        void show();
        //Interface* query(uint32_t daddr);//暂时弃用
        void write_routing();
};

#endif