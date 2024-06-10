#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <thread>
typedef struct INTERFACE {
    const char* name;
    std::thread thread;
} Interface;

#endif // PACKET_MANAGE_H