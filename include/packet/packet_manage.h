#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "../interface/interface.h"

void create_hello_thread(Interface *interface);
void create_recv_thread(Interface *interface);



#endif // PACKET_MANAGE_H
