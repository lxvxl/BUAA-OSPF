#ifndef ROUTING_H
#define ROUTING_H

#include <vector>
#include <map>
#include "../packet/packets.h"
#include <stdint.h>
#include <set>
#include <unordered_map>
#include <set>
#include "../db/lsa_db.h"

class LSADatabase;
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
        bool        inner;
        NetNode(uint32_t ip, uint32_t mask, bool inner) : Node(false, ip), mask(mask), inner(inner) {};
        void show() override;
        ~NetNode() override = default;
};



class RoutingManager {
    class RoutingTable {
    public:
        class RoutingItem {
        public:
            uint32_t target_net;
            uint32_t mask;
            uint32_t next_hop;
            bool     inner;
            int      metric;

            RoutingItem(uint32_t t_net, uint32_t mask, uint32_t next_hop, bool inner, int metric)
                : target_net(t_net), mask(mask), next_hop(next_hop), inner(inner), metric(metric) {}
        };

        std::unordered_map<uint64_t, RoutingItem> items;

        bool insert(uint32_t target_net, uint32_t mask, uint32_t next_hop, bool inner, int metric);
        void remove(uint32_t target_net, uint32_t mask);
    };
    private:
        std::unordered_map<uint32_t, RouterNode*> router_node_map;
        std::unordered_map<uint32_t, NetNode*> net_node_map;
        RouterNode *me;

        RouterNode* get_router_node(uint32_t id);
        void dijkstra();
        void reset();
    public:
        RoutingTable routing_table;
        void generate(LSADatabase &db);
        void show();
};

#endif