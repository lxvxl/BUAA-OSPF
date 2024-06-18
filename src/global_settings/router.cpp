#include "../../include/global_settings/router.h"
std::vector<Interface*> router::interfaces;
uint32_t router::router_id = inet_addr("2.2.2.2");
LSADatabase router::lsa_db;

const uint8_t router::config::options = 2;
const uint16_t router::config::MTU = 1500;