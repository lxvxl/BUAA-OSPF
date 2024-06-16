#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <thread>
#include <arpa/inet.h>
#include "../neighbor/neighbor.h"
#include <vector>
#include "../packet/packets.h"
#include <mutex>
#include "../db/lsa_db.h"
#define BUFFER_SIZE 4096

struct Neighbor;
struct LSADatabase;

enum InterfaceState {
    DOWN,
    LOOPBACK,
    WAITING,
    POINT2POINT,
    DROTHER,
    BACKUP,
    DR
};
struct Interface {
    const char* name;
    std::thread recv_thread;
    std::thread send_thread;
    LSADatabase *db;

    InterfaceState state                    = InterfaceState::DOWN;
    uint8_t     rtr_priority                = 1; 
    uint32_t    ip                          = inet_addr("192.168.64.20");
    uint32_t    network_mask                = inet_addr("255.255.255.0");
    uint32_t    area_id                     = 0;
    uint16_t    hello_interval              = 10;
    uint32_t    dead_interval               = 40;
    uint32_t    designated_route            = 0; // 这里用的是网络字节序
    uint32_t    backup_designated_router    = 0;     
    uint32_t    router_id                   = inet_addr("2.2.2.2");
    uint32_t    dr                          = inet_addr("0.0.0.0");
    uint32_t    bdr                         = inet_addr("0.0.0.0");
    uint32_t    interface_output_cost       = 0;
    uint32_t    rxmt_interval               = 10;

    uint32_t    wait_timer                  = -1;//使接口退出waiting状态的单击计时器
    uint32_t    hello_timer                 = -1;

    int         send_socket_fd;
    int         recv_socket_fd;

    std::vector<Neighbor *> neighbors;
    std::mutex mtx;

    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    Interface(const char *name);

    void        event_interface_up();
    void        event_wait_timer();
    void        event_backup_seen();
    void        event_neighbor_change();
    void        event_loop_ind();
    void        event_unloop_ind();
    void        event_interface_down();

    Neighbor*   get_neighbor(uint32_t router_id);
    void        send_thread_runner();
    void        send_hello_packet();
    void        send_dd_packet(Neighbor *neighbor);
    void        send_last_dd_packet(Neighbor *neighbor);
    void        elect_dr();
    void        recv_thread_runner();
};
#endif 