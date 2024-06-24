#ifndef LOGGER_H
#define LOGGER_H

#include "../interface/interface.h"
#include "../neighbor/neighbor.h"
#include <string>

namespace logger {
    void event_log(Interface *interface, const std::string &event);
    void event_log(Neighbor *neighbor, const std::string &event);

    void state_transition_log(Interface *interface, InterfaceState state1, InterfaceState state2);
    void state_transition_log(Neighbor *neighbor, NeighborState state1, NeighborState state2);

    void other_log(Interface *interface, const std::string &infos);
    void other_log(Neighbor *neighbor, const std::string &infos);

    void enable_terminal_logging(bool enable);
}

#endif