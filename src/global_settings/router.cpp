#include "../../include/global_settings/router.h"
std::vector<Interface*> router::interfaces;

const uint8_t router::config::options = 2;
const uint16_t router::config::MTU = 1500;