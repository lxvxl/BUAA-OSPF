#include "../include/interface/interface.h"
#include <unistd.h>
#include <stdio.h>
#include <system_error>
#include <iostream>
#include "../include/global_settings/router.h"
#include "../include/global_settings/common.h"
#include "../include/logger/logger.h"

int main() {
    //Interface interface1("enp0s3", inet_addr("192.168.64.20"), inet_addr("255.255.255.0"));
    //interface1.event_interface_up();
    Interface interface2("enp0s9", inet_addr("40.1.1.1"), inet_addr("255.255.255.0"), 100);
    interface2.event_interface_up();
    Interface interface3("enp0s10", inet_addr("30.1.1.2"), inet_addr("255.255.255.0"), 200);
    interface3.event_interface_up();
    while (true) {   
        std::string inst;
        std::getline(std::cin, inst);
        LSADatabase &lsdb = router::lsa_db;
        if (inst == "dis db") {
            std::unique_lock<std::mutex> lock(router::mutex);
            router::lsa_db.show();
            lock.unlock();
        } else if (inst == "dis routing") {
            std::unique_lock<std::mutex> lock(router::mutex);
            router::routing_table.generate(lsdb.router_lsas, lsdb.network_lsas);
            router::routing_table.show();
            lock.unlock();    
        } else if (inst == "quit") {
            break;
        } else if (inst == "disable log") { 
            logger::enable_terminal_logging(false);
        } else if (inst == "enable log") {
            logger::enable_terminal_logging(true);
        } else if (inst == "write routing table") { 
            std::unique_lock<std::mutex> lock(router::mutex);
            router::routing_table.write_routing();
            lock.unlock();  
        } else {
            std::cout<<"illegal instruction!"<<std::endl;
        }
    }
    
    return 0;
}
