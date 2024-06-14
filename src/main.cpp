#include "../include/interface/interface.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>

int main() {
    Interface interface;
    interface.name = "enp0s3";
    interface.event_interface_up();
    while (true)
    {
        sleep(10);
    }
    
    return 0;
}
