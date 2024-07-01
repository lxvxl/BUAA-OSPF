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

enum InterfaceState : int{
    DOWN,
    LOOPBACK,
    WAITING,
    POINT2POINT,
    DROTHER,
    BACKUP,
    DR
};
struct Interface {
    const char* name                        ;
    std::thread recv_thread;
    std::thread main_thread;

    InterfaceState state                    = InterfaceState::DOWN;
    uint8_t     rtr_priority                = 1; 
    uint32_t    ip                          ;
    uint32_t    network_mask                ;
    uint32_t    area_id                     = 0;
    uint16_t    hello_interval              = 10;
    uint32_t    dead_interval               = 40;
    uint32_t    dr                          = inet_addr("0.0.0.0");
    uint32_t    bdr                         = inet_addr("0.0.0.0");
    uint32_t    rxmt_interval               = 10;
    uint16_t    metric                      ;

    uint32_t    wait_timer                  = -1;//使接口退出waiting状态的单击计时器
    uint32_t    hello_timer                 = -1;

    int         send_socket_fd              = 0;
    int         recv_socket_fd              = 0;

    std::vector<Neighbor *> neighbors;

    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];

    Interface(const char *name, uint32_t ip, uint32_t mask, uint16_t metric);

    void        event_interface_up();
    void        event_wait_timer();
    void        event_backup_seen();
    void        event_neighbor_change();
    void        event_loop_ind();
    void        event_unloop_ind();
    void        event_interface_down();

    Neighbor*   get_neighbor_by_id(uint32_t router_id);
    Neighbor*   get_neighbor_by_ip(uint32_t ip);
    void        main_thread_runner();

    void        send_hello_packet();
    void        send_dd_packet(Neighbor *neighbor);
    void        send_last_dd_packet(Neighbor *neighbor);
    void        send_lsr_packet(Neighbor *neighbor);
    void        send_lsu_packet(std::vector<LSAHeader*>& r_lsas, uint32_t dst_addr);
    void        send_lsu_packet(LSAHeader *r_lsa, uint32_t dst_addr);
    void        send_lsack_packet(std::vector<LSAHeader*>& v_lsas, uint32_t dst_addr);
    void        send_lsack_packet(LSAHeader *v_lsa, uint32_t dst_addr);

    void        elect_dr();
    void        recv_thread_runner();

    //清除失效的LSA
    void        clear_invalid_req(LSAHeader *old_r_lsa, LSAHeader *new_r_lsa);
};
#endif 