#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works


uint16_t CONTROL_PORT;
uint16_t ROUTER_PORT;
uint16_t DATA_PORT;
uint32_t IP_ADDRESS;
uint16_t NO_OF_ROUTERS;
uint16_t UPDATE_INTERVAL;

struct timeval cst1,cst2;

bool router_port_initialised;
bool init_received;
struct  timeval tv;

struct __attribute__((__packed__)) ROUTER_INFO
{
    unsigned char router_id[2];
    unsigned char router_port1[2];
    unsigned char router_port2[2];
    unsigned char cost[2];
    unsigned char ip_addr[4];
    unsigned char init_cost[2];

};

struct __attribute__((__packed__)) ROUTING_TABLE
{
    unsigned char router_id[2];
    unsigned char padding[2];
    unsigned char next_hop_id[2];
    unsigned char cost[2];
    unsigned char router_port[2];
    unsigned char data_port[2];
    unsigned char ip_addr[4];
    unsigned char init_cost[2];
    bool isneighbour;
};
struct ROUTER_INFO router_info[5];
struct ROUTING_TABLE routing_table[5];

struct __attribute__((__packed__)) ROUTING_UPDATE
{
    unsigned char router_id[2];
    unsigned char cost[2];
};
struct  ROUTING_UPDATE routing_update[5];


#endif