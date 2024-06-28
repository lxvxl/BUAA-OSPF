#include "../../include/routing/routing.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"

#include <unordered_map>
#include <queue>
#include <bitset>
#include <string>

RouterNode* RoutingManager::get_router_node(uint32_t id) {
    if (router_node_map.find(id) == router_node_map.end()) {
        router_node_map[id] = new RouterNode(id);
    }
    return router_node_map[id];
}



void RoutingManager::generate(LSADatabase &db) {
    reset();
    // Process NetworkLSAs
    for (NetworkLSA* network_lsa : db.network_lsas) {
        net_node_map[network_lsa->header.link_state_id] = new NetNode(network_lsa->header.link_state_id, network_lsa->network_mask, true);
    }
    // Process RouterLSAs
    for (RouterLSA* router_lsa : db.router_lsas) {
        RouterNode *router_node = get_router_node(router_lsa->header.link_state_id);
        for (int i = 0; i < router_lsa->get_link_num(); ++i) {
            RouterLSALink& link = router_lsa->links[i];
            NetNode* net_node;
            if (link.type == TRAN) {
                net_node = net_node_map[link.link_id];
                router_node->neighbor_2_interface[net_node] = link.link_data;
            } else if (link.type == STUB) {
                net_node = new NetNode(link.link_id, link.link_data, true);
            }
            net_node->neighbor_nodes[router_node] = 0;
            router_node->neighbor_nodes[net_node] = link.metric;
        }
    }
    me = get_router_node(router::router_id);
    dijkstra();
}

void RoutingManager::dijkstra() {
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
        if (node == me || previous.find(node) == previous.end() || node->is_router) {
            continue;
        }
        NetNode *target = (NetNode*)node;
        Node* step = target;
        RouterNode *next_router_node = NULL;
        int metric = 0;
        while (previous[step] != me) {
            metric += previous[step]->neighbor_nodes[step];
            step = previous[step];
            if (step->is_router) {
                next_router_node = (RouterNode*)step;
            }
        }
        if (next_router_node != NULL) {
            routing_table.insert(target->id & target->mask, 
                                 target->mask, 
                                 next_router_node->neighbor_2_interface[step], 
                                 target->inner, 
                                 metric + me->neighbor_nodes[step]);
        }
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

void RoutingManager::show() {
    std::cout<<"=====Routing Table====="<<std::endl;
    for (const auto& [key, item] : routing_table.items) {
        in_addr target_net_addr = {item.target_net};
        in_addr mask_addr = {item.mask};
        in_addr next_hop_addr = {item.next_hop};

        std::cout << "Target Network: " << inet_ntoa({item.target_net})
                  << ", Mask: " << inet_ntoa({item.mask})
                  << ", Next Hop: " << inet_ntoa(next_hop_addr)
                  << ", Inner: " << (item.inner ? "Yes" : "No")
                  << ", Metric: " << item.metric << std::endl;
    }
}

void RoutingManager::reset() {
    for (auto pair : router_node_map) {
        delete pair.second;
    }
    for (auto pair : net_node_map) {
        delete pair.second;
    }
    router_node_map.clear();
    net_node_map.clear();
}