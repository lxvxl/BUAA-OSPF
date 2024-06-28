#include "../../include/routing/routing.h"
#include "../../include/global_settings/router.h"
#include "../../include/global_settings/common.h"

uint64_t make_key(uint32_t target_net, uint32_t mask) {
    return (static_cast<uint64_t>(target_net) << 32) | mask;
}

bool RoutingManager::RoutingTable::insert(uint32_t target_net, uint32_t mask, uint32_t next_hop, bool inner, int metric) {
    uint64_t key = make_key(target_net, mask);
    RoutingItem item(target_net, mask, next_hop, inner, metric);
    auto result = items.insert({key, item});

    std::string command(std::string("sudo route add -net ") + inet_ntoa({target_net}) + " netmask " + inet_ntoa({mask}) + " gw " + inet_ntoa({next_hop}));
    system(command.c_str());
    return result.second;
}

void RoutingManager::RoutingTable::remove(uint32_t target_net, uint32_t mask) {
    std::string command(std::string("sudo route del -net ") + inet_ntoa({target_net}) + " netmask " + inet_ntoa({mask}));
    system(command.c_str());
    uint64_t key = make_key(target_net, mask);
    items.erase(key);
}