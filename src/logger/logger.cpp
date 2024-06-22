#include "../../include/logger/logger.h"
#include "../../include/global_settings/common.h"
#include <iostream>
#include <map>


auto& out = std::cout;
std::map<InterfaceState, std::string> interface_state_map = {
    {DOWN, "DOWN"},
    {LOOPBACK, "LOOPBACK"},
    {WAITING, "WAITING"},
    {POINT2POINT, "POINT2POINT"},
    {DROTHER, "DROTHER"},
    {BACKUP, "BACKUP"},
    {DR, "DR"}
};

std::map<NeighborState, std::string> neighbor_state_map = {
    {N_DOWN, "DOWN"},
    {ATTEMPT, "ATTEMPT"},
    {INIT, "INIT"},
    {_2WAY, "2WAY"},
    {EXSTART, "EXSTART"},
    {EXCHANGE, "EXCHANGE"},
    {LOADING, "LOADING"},
    {FULL, "FULL"}
};

void logger::event_log(Interface *interface, std::string event) {
    out<<"\r[Interface "<<interface->name<<"]: "<<event<<std::endl;
}
void logger::event_log(Neighbor *neighbor, std::string event) {
    out<<"\r[Neighbor "<<neighbor->ip<<"]: "<<event<<std::endl;
}

void logger::state_transition_log(Interface *interface, InterfaceState state1, InterfaceState state2) {
    out<<"\r[Interface "<<interface->name<<"]: "<<interface_state_map[state1]<<" --> "<<interface_state_map[state2]<<std::endl;
}
 
void logger::state_transition_log(Neighbor *neighbor, NeighborState state1, NeighborState state2) {
    out<<"\r[Neighbor "<<neighbor->ip<<"]: "<<neighbor_state_map[state1]<<" --> "<<neighbor_state_map[state2]<<std::endl;
}

void logger::other_log(Interface *interface, std::string infos) {
    out<<"\r[Interface "<<interface->name<<"]: "<<infos<<std::endl;

}

void logger::other_log(Neighbor *neighbor, std::string infos) {
    out<<"\r[Neighbor "<<neighbor->ip<<"]: "<<infos<<std::endl;
}