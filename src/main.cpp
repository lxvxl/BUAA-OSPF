#include "../include/interface/interface.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>
#include "../include/global_settings/router.h"

int main() {
    Interface interface("enp0s3");
    router::interfaces.push_back(&interface);
    interface.event_interface_up();
    while (true)
    {
        sleep(10);
    }
    
    return 0;
}
