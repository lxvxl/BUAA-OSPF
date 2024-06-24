#include "../include/interface/interface.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>
#include "../include/global_settings/router.h"
#include "../include/global_settings/common.h"

int main() {
    Interface interface("enp0s3");
    router::interfaces.push_back(&interface);
    interface.event_interface_up();
    while (true)
    {   
        std::string inst;
        std::getline(std::cin, inst);
        if (inst == "dis db") {
            std::unique_lock<std::mutex> lock(router::mutex);
            router::lsa_db.show();
            lock.unlock();
        } else if (inst == "dis routing") {
            std::unique_lock<std::mutex> lock(router::mutex);
            router::routing_table.show();
            lock.unlock();    
        } else if (inst == "quit") {
            break;
        } else {
            std::cout<<"illegal instruction!"<<std::endl;
        }
    }
    
    return 0;
}
