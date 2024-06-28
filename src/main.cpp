#include "../include/interface/interface.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>
#include "../include/global_settings/router.h"
#include "../include/global_settings/common.h"
#include "../include/logger/logger.h"
#include <unordered_map>

int main() {
    //Interface interface1("enp0s3", inet_addr("192.168.64.20"), inet_addr("255.255.255.0"));
    //interface1.event_interface_up();
    //Interface interface2("enp0s9", inet_addr("192.168.65.20"), inet_addr("255.255.255.0"));
    //interface2.event_interface_up();
    Interface interface3("enp0s10", inet_addr("192.168.67.20"), inet_addr("255.255.255.0"), inet_addr("0.0.0.0"));
    interface3.event_interface_up();
    while (true) {   
        std::string inst;
        std::getline(std::cin, inst);
        if (inst == "dis db") {
            std::unique_lock<std::mutex> lock(router::mutex);
            for (auto& pair : router::area_lsa_dbs) {
                std::cout<<"Area "<<inet_ntoa({pair.first})<<std::endl;
                pair.second.show();
            }
            lock.unlock();
        } else if (inst == "dis routing") {
            std::unique_lock<std::mutex> lock(router::mutex);
            for (auto& pair : router::area_lsa_dbs) {
                std::cout<<"Area "<<inet_ntoa({pair.first})<<std::endl;
                pair.second.routing_manager->show();
            }
            lock.unlock();    
        } else if (inst == "quit") {
            break;
        } else if (inst == "disable log") { 
            logger::enable_terminal_logging(false);
        } else if (inst == "enable log") {
            logger::enable_terminal_logging(true);
        } else if (inst == "write routing table") { 
            std::unique_lock<std::mutex> lock(router::mutex);
            //router::routing_table.write_routing();
            lock.unlock();  
        } else {
            std::cout<<"illegal instruction!"<<std::endl;
        }
    }
    
    return 0;
}
