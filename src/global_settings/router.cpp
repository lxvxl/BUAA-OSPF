#include "../../include/global_settings/router.h"
std::vector<Interface*> router::interfaces;
uint32_t router::router_id = inet_addr("3.3.3.3");
LSADatabase router::lsa_db;
std::mutex router::mutex;
RoutingTable router::routing_table;

const uint8_t router::config::options = 0x2;
const uint16_t router::config::MTU = 1500;
const int router::config::min_ls_arrival = 1;