#include "../../include/packet/packets.h"
#include "../../include/global_settings/common.h"

bool LSAHeader::operator==(LSAHeader &another) {
    return memcmp(this, &another, sizeof(LSAHeader));
}