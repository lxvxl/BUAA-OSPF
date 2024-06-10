#include "../include/interface/interface.h"
#include "../include/packet/packet_manage.h"
#include <unistd.h>

int main() {
    Interface interface;
    interface.name = "eth0";
    create_recv_thread(&interface);
    while (true)
    {
        sleep(10);
    }
    
    return 0;
}
