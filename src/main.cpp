#include "../include/interface/interface.h"
#include "../include/packet/packet_manage.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>

int main() {
    Interface interface;
    interface.name = "enp0s3";
    try {
        create_hello_thread(&interface);
        create_recv_thread(&interface);
    } catch (const std::system_error e) {
        std::cerr << "Thread creation failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    while (true)
    {
        sleep(10);
    }
    
    return 0;
}
