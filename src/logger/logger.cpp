#include "../../include/logger/logger.h"
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <arpa/inet.h>

namespace {
    bool terminal_logging_enabled = true;
    std::ofstream log_file("log.txt", std::ios::trunc);
    std::mutex log_mutex; // To ensure thread-safe logging

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

    void log_message(const std::string &message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (terminal_logging_enabled) {
            std::cout << message << std::endl;
        }
        log_file << message << std::endl;
    }
}

void logger::enable_terminal_logging(bool enable) {
    terminal_logging_enabled = enable;
}

void logger::event_log(Interface *interface, const std::string &event) {
    std::string log_entry = "[Interface " + std::string(interface->name) + "]: " + event;
    log_message(log_entry);
}

void logger::event_log(Neighbor *neighbor, const std::string &event) {
    std::string log_entry = "[Neighbor " + std::string(inet_ntoa({neighbor->ip})) + "]: " + event;
    log_message(log_entry);
}

void logger::state_transition_log(Interface *interface, InterfaceState state1, InterfaceState state2) {
    std::string log_entry = "[Interface " + std::string(interface->name) + "]: " + interface_state_map[state1] + " --> " + interface_state_map[state2];
    log_message(log_entry);
}

void logger::state_transition_log(Neighbor *neighbor, NeighborState state1, NeighborState state2) {
    std::string log_entry = "[Neighbor " + std::string(inet_ntoa({neighbor->ip})) + "]: " + neighbor_state_map[state1] + " --> " + neighbor_state_map[state2];
    log_message(log_entry);
}

void logger::other_log(Interface *interface, const std::string &infos) {
    std::string log_entry = "[Interface " + std::string(interface->name) + "]: " + infos;
    log_message(log_entry);
}

void logger::other_log(Neighbor *neighbor, const std::string &infos) {
    std::string log_entry = "[Neighbor " + std::string(inet_ntoa({neighbor->ip})) + "]: " + infos;
    log_message(log_entry);
}
