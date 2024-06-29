#include "../../include/routing/routing.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"

#include <unordered_map>
#include <queue>
#include <bitset>
#include <string>

RouterNode* RoutingTable::get_router_node(uint32_t id) {
    if (router_node_map.find(id) == router_node_map.end()) {
        router_node_map[id] = new RouterNode(id);
    }
    return router_node_map[id];
}

NetNode* RoutingTable::get_net_node(uint32_t ip, uint32_t mask) {
    ip = ip & mask;
    if (net_node_map.find(ip) == net_node_map.end()) {
        net_node_map[ip] = new NetNode(ip, mask);
    }
    return net_node_map[ip];
}


//void RoutingTable::generate(std::vector<RouterLSA*>& router_lsas, std::vector<NetworkLSA*>& network_lsas) {
//    reset();
//    // Process NetworkLSAs
//    for (NetworkLSA* network_lsa : network_lsas) {
//        get_net_node(network_lsa->header.link_state_id, network_lsa->network_mask);
//        for (int i = 0; i < network_lsa->get_routers_num(); ++i) {
//            uint32_t attached_router_id = network_lsa->attached_routers[i];
//            get_router_node(attached_router_id);
//        }
//    }
//    // Process RouterLSAs
//    for (RouterLSA* router_lsa : router_lsas) {
//        RouterNode *router_node = get_router_node(router_lsa->header.link_state_id);
//        for (int i = 0; i < router_lsa->get_link_num(); ++i) {
//            RouterLSALink& link = router_lsa->links[i];
//            NetNode* net_node = get_net_node(link.link_id, link.type == TRAN ? 0x00ffffff : link.link_data);
//            net_node->neighbor_nodes[router_node] = link.metric;
//            router_node->neighbor_nodes[net_node] = link.metric;
//            router_node->neighbor_2_interface[net_node] = link.type == TRAN ? link.link_data : link.link_id;
//        }
//    }
//    me = get_router_node(router::router_id);
//    dijkstra();
//}

void RoutingTable::generate(std::vector<RouterLSA*>& router_lsas, std::vector<NetworkLSA*>& network_lsas) {
    reset();
    //std::cout<<"generating routing table...\n";
    // Process NetworkLSAs
    for (NetworkLSA* network_lsa : network_lsas) {
        net_node_map[network_lsa->header.link_state_id] = new NetNode(network_lsa->header.link_state_id, network_lsa->network_mask);
        //std::cout<<"netnode "<<inet_ntoa({network_lsa->header.link_state_id})<<std::endl;
    }
    // Process RouterLSAs
    for (RouterLSA* router_lsa : router_lsas) {
        RouterNode *router_node = get_router_node(router_lsa->header.link_state_id);
        for (int i = 0; i < router_lsa->get_link_num(); ++i) {
            RouterLSALink& link = router_lsa->links[i];
            NetNode* net_node;
            if (link.type == TRAN) {
                if (net_node_map.find(link.link_id) == net_node_map.end()) {
                    net_node_map[link.link_id] = new NetNode(link.link_id, 0x00ffffff);
                }
                net_node = net_node_map[link.link_id];
                router_node->neighbor_2_interface[net_node] = link.link_data;
            } else if (link.type == STUB) {
                std::cout<<"STUB Net"<<std::endl;
                if (net_node_map.find(link.link_id & link.link_data) == net_node_map.end()) {
                    net_node = new NetNode(link.link_id, link.link_data);
                    net_node_map[link.link_id & link.link_data] = net_node;
                } else {
                    net_node = net_node_map[link.link_id & link.link_data];
                }
            } else if (link.type == P2P) {
                continue;
                net_node = new NetNode(link.link_data, 0xffffff00);
            }
            net_node->neighbor_nodes[router_node] = 0;
            router_node->neighbor_nodes[net_node] = link.metric;
        }
    }
    me = get_router_node(router::router_id);
    dijkstra();
}

void RoutingTable::dijkstra() {
    std::cout<<"dijkstra\n"<<std::endl;
    for (auto pair : net_node_map) {
        pair.second->show();
        for (auto n : pair.second->neighbor_nodes) {
            n.first->show();
        }
        std::cout<<std::endl;
    }
    for (auto pair : router_node_map) {
        pair.second->show();
        for (auto n : pair.second->neighbor_nodes) {
            n.first->show();
        }
        std::cout<<std::endl;
    }
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
    //for (auto& pair : distances) {
    //    Node* node = pair.first;
    //    if (node == me || previous.find(node) == previous.end()) {
    //        continue;
    //    }
//
    //    Node* step = node;
//
    //    while (previous[step] != me) {
    //        step = previous[step];
    //    }
    //    next_step[node] = step;
    //}
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
            target->show();
            next_router_node->show();
            std::cout<<std::endl;
            next_step[target] = next_router_node->neighbor_2_interface[step];
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

void RoutingTable::show() {
    std::cout<<"=====Routing Table====="<<std::endl;
    for (auto pair : next_step) {
        pair.first->show();
        std::cout<<'\t';
        std::cout<<inet_ntoa({pair.second})<<std::endl;
    }
    //for (auto pair : net_node_map) {
    //    pair.second->show();
    //    std::cout<<std::endl;
    //}
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

//Interface* RoutingTable::query(uint32_t daddr) {
//    for (auto pair : next_step) {
//        if (pair.first->is_router) {
//            continue;
//        }
//        NetNode *target = (NetNode*)pair.first;
//        if ((target->id & target->mask) == (daddr & target->mask)) {
//            uint32_t next_addr = pair.second->id;
//            for (Interface *interface : router::interfaces) {
//                if ((interface->ip & interface->network_mask) == (next_addr & interface->network_mask)) {
//                    return interface;
//                }
//            }
//            return NULL;
//        }
//    }
//    return NULL;
//}

void RoutingTable::add_route(const std::string& target_net, const std::string& target_mask, const std::string& next_net) {
    std::string command(std::string("sudo route add -net ") + target_net + " netmask " + target_mask + " gw " + next_net);
    int result = system(command.c_str());
    
    if (result != 0) {
        std::cerr << "Failed to add route. Command: " << command << std::endl;
    } else {
        std::cout << "Route added successfully. Command: " << command << std::endl;
        //written_routes.insert(command);
    }
}

void RoutingTable::write_routing() {
    //for (std::string* route_item : written_routes) {
    //    route_item->replace(route_item->find("add"), 3, "del");
    //    system(route_item->c_str());
    //}
    written_routes.clear();
    for (auto pair : next_step) {
        if (pair.first->is_router) {
            continue;
        }
        NetNode *target = (NetNode*)pair.first;
        std::string target_network = inet_ntoa({target->id & target->mask});
        std::string target_mask = inet_ntoa({target->mask});
        std::string next_step = inet_ntoa({pair.second});
        add_route(target_network, target_mask, next_step);
    }
}