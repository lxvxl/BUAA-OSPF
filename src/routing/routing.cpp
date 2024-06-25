#include "../../include/routing/routing.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"

#include <unordered_map>
#include <queue>
#include <bitset>

RouterNode* RoutingTable::get_router_node(uint32_t id) {
    if (router_node_map.find(id) == router_node_map.end()) {
        router_node_map[id] = new RouterNode(id);
    }
    return router_node_map[id];
}

NetNode* RoutingTable::get_net_node(uint32_t ip, uint32_t mask) {
    if (net_node_map.find(ip) == net_node_map.end()) {
        net_node_map[ip] = new NetNode(ip, mask);
    }
    return net_node_map[ip];
}


void RoutingTable::generate(std::vector<RouterLSA*>& router_lsas, std::vector<NetworkLSA*>& network_lsas) {
    reset();
    // Process NetworkLSAs
    for (NetworkLSA* network_lsa : network_lsas) {
        NetNode *net_node = get_net_node(network_lsa->header.link_state_id, network_lsa->network_mask);
        for (int i = 0; i < network_lsa->get_routers_num(); ++i) {
            uint32_t attached_router_id = network_lsa->attached_routers[i];
            RouterNode *router_node = get_router_node(attached_router_id);
        }
    }
    // Process RouterLSAs
    for (RouterLSA* router_lsa : router_lsas) {
        RouterNode *router_node = get_router_node(router_lsa->header.link_state_id);
        for (int i = 0; i < router_lsa->get_link_num(); ++i) {
            RouterLSALink& link = router_lsa->links[i];
            NetNode* net_node = get_net_node(link.link_id, link.type == TRAN ? 0 : link.link_data);
            net_node->neighbor_nodes[router_node] = link.metric;
            router_node->neighbor_nodes[net_node] = link.metric;
            router_node->neighbor_2_interface[net_node] = link.type == TRAN ? link.link_data : link.link_id;
        }
    }
    me = get_router_node(router::router_id);
    dijkstra();
}

void RoutingTable::dijkstra() {
    std::unordered_map<Node*, uint32_t> distances;
    std::unordered_map<Node*, Node*> previous;
    std::priority_queue<std::pair<uint32_t, Node*>, std::vector<std::pair<uint32_t, Node*>>, std::greater<>> priority_queue;

    // Initialize distances
    for (auto& pair : router_node_map) {
        distances[pair.second] = std::numeric_limits<uint32_t>::max();
    }
    for (auto& pair : net_node_map) {
        distances[pair.second] = std::numeric_limits<uint32_t>::max();
    }
    distances[me] = 0;

    priority_queue.push({0, me});

    while (!priority_queue.empty()) {
        Node* current = priority_queue.top().second;
        uint32_t current_distance = priority_queue.top().first;
        priority_queue.pop();

        if (current_distance > distances[current]) {
            continue;
        }

        for (auto& neighbor_pair : current->neighbor_nodes) {
            Node* neighbor = neighbor_pair.first;
            uint16_t weight = neighbor_pair.second;
            uint32_t new_distance = current_distance + weight;

            if (new_distance < distances[neighbor]) {
                distances[neighbor] = new_distance;
                previous[neighbor] = current;
                priority_queue.push({new_distance, neighbor});
            }
        }
    }
    // Fill the next_step map
    for (auto& pair : distances) {
        Node* node = pair.first;
        if (node == me || previous.find(node) == previous.end()) {
            continue;
        }

        Node* step = node;

        while (previous[step] != me) {
            step = previous[step];
        }
        next_step[node] = step;
    }
}

void RouterNode::show() {
    std::cout<<"Router["<<inet_ntoa({this->id})<<"]";
}

void NetNode::show() {
    std::cout<<"Net["<<inet_ntoa({this->id});
    std::bitset<32> binary_mask(ntohl(mask));
    int prefix_length = 0;
    for (int i = 31; i >= 0; --i) {
        if (binary_mask[i]) {
            ++prefix_length;
        } else {
            break;
        }
    }
    std::cout<<" "<<inet_ntoa({this->mask})<<"]";
}

void RoutingTable::show() {
    std::cout<<"=====Routing Table====="<<std::endl;
    for (auto pair : next_step) {
        pair.first->show();
        std::cout<<'\t';
        pair.second->show();
        std::cout<<std::endl;
    }
}

void RoutingTable::reset() {
    for (auto pair : router_node_map) {
        delete pair.second;
    }
    for (auto pair : net_node_map) {
        delete pair.second;
    }
    router_node_map.clear();
    net_node_map.clear();
    next_step.clear();
}

Interface* RoutingTable::query(uint32_t daddr) {
    for (auto pair : next_step) {
        if (pair.first->is_router) {
            continue;
        }
        NetNode *target = (NetNode*)pair.first;
        if ((target->id & target->mask) == (daddr & target->mask)) {
            uint32_t next_addr = pair.second->id;
            for (Interface *interface : router::interfaces) {
                if ((interface->ip & interface->network_mask) == (daddr & interface->network_mask)) {
                    return interface;
                }
            }
            return NULL;
        }
    }
    return NULL;
}