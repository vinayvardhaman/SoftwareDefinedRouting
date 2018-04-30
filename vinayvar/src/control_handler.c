/**
 * @control_handler
 * @author  vinay vardhaman <vinayvar@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/author.h"
#include "../include/init.h"
#include <arpa/inet.h>

#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

#define MAXBUFLEN 100

/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&control_conn_list);

    return sock;
}

int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

bool control_recv_hook(int sock_index)
{
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;
    bool terminate = FALSE;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);

    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return FALSE;
    }

    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_HEADER) == 8) 
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */
        BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.

        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
    #endif
    #ifndef PACKET_USING_STRUCT
        memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
        memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
        payload_len = ntohs(payload_len);
    #endif

    free(cntrl_header);

    /* Get control payload */
    if(payload_len != 0){
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);

        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return FALSE;
        }
    }

    /* Triage on control_code */
    switch(control_code){
        case 0: author_response(sock_index);
                break;

        case 1: init_response(sock_index, cntrl_payload);
                break;

        case 2: routing_table_response(sock_index);
                break;

        case 3: update_response(sock_index,cntrl_payload);
                break;

        case 4: crash_response(sock_index);
                terminate = TRUE;
                break;

    }

    if(payload_len != 0) free(cntrl_payload);

    if(terminate)
        exit(0);


    return TRUE;
}


int create_router_sock()
{
    int sock;
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(sock < 0)
    ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
    ERROR("setsockopt() failed");

    bzero(&router_addr, sizeof(router_addr));

    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = htons(ROUTER_PORT);

    if(bind(sock, (struct sockaddr *)&router_addr, sizeof(router_addr)) < 0)
    ERROR("bind() failed");

    printf("Router socket created for Port:");
    printf("%" PRIu16 "\n",ROUTER_PORT);

    return sock;
}

void send_routing_updates()
{
    printf("In Send Updates\n");
    int sockfd;
    struct sockaddr_in addrin;
    int len;
    char s[INET_ADDRSTRLEN];

    uint16_t table_len;
    uint16_t no_of_updates = htons(NO_OF_ROUTERS);
    uint16_t srp = htons(ROUTER_PORT);
    uint32_t srip = htonl(IP_ADDRESS);
    char *routing_table_update;

    table_len = 8 + NO_OF_ROUTERS* 12;
    routing_table_update = (char*)malloc(table_len);

    memcpy(routing_table_update, &no_of_updates,2);
    memcpy(routing_table_update+2, &srp,2);
    memcpy(routing_table_update+4, &srip,4);

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        memcpy(routing_table_update+8+i*12, routing_table[i].ip_addr, 4);
        memcpy(routing_table_update+8+i*12+4, routing_table[i].router_port, 2);
        memcpy(routing_table_update+8+i*12+6, routing_table[i].padding,2);
        memcpy(routing_table_update+8+i*12+8, routing_table[i].router_id, 2);
        memcpy(routing_table_update+8+i*12+10, routing_table[i].cost, 2);

    }

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        if(routing_table[i].isneighbour)
        {
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);

            if(sockfd < 0)
            ERROR("socket() failed");

            addrin.sin_family = AF_INET;
            memcpy(&addrin.sin_addr.s_addr,routing_table[i].ip_addr,4);
            memcpy(&addrin.sin_port,routing_table[i].router_port,2);
            len = sizeof(addrin);

            int num = sendto(sockfd, routing_table_update, table_len, 0, (struct sockaddr *)&addrin, len);
            printf("Bytes sent for rupdate:%d\n",num);
            inet_ntop(AF_INET,&(addrin.sin_addr),s,INET_ADDRSTRLEN);

            printf("Sent rupdate to:%s\n",s);

        }
    }

    free(routing_table_update);
    close(sockfd);

}

void receive_routing_updates(int sockfd)
{
    printf("In Receive\n");
    int numbytes;
    struct sockaddr their_addr;
    char buf[MAXBUFLEN];
    char s[INET_ADDRSTRLEN];

    int addr_len = sizeof(their_addr);
    int num = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, &their_addr, &addr_len);

    printf("listener: got packet from %s\n",
           inet_ntop(AF_INET,&their_addr,
                     s, INET_ADDRSTRLEN));
    printf("Bytes received as updates:%d\n",num);
    // fill routing_update_struct

    uint16_t no_of_updates;
    uint16_t cost_to_neighbour,temp;
    uint16_t neighbour_id;
    memcpy(&no_of_updates,buf,2);
    no_of_updates = ntohs(no_of_updates);

    for(int i=0; i<no_of_updates; i++)
    {
        memcpy(routing_update[i].router_id,buf+8+i*12+8,2);
        memcpy(routing_update[i].cost,buf+8+i*12+10,2);

        temp = routing_update[i].cost[0]*256 + routing_update[i].cost[1];

        if(temp == 0)
        {
            memcpy(&neighbour_id,routing_update[i].router_id,2);

            for(int j=0; j<NO_OF_ROUTERS; j++)
            {
                if(memcmp(routing_update[i].router_id,routing_table[j].router_id,2)==0)
                {
                    memcpy(&cost_to_neighbour, routing_table[j].init_cost,2);
                    cost_to_neighbour = ntohs(cost_to_neighbour);
                }
            }
        }
    }

    printf("Update from neighbour:%d with cost%d\n",ntohs(neighbour_id),cost_to_neighbour);

    // update routing_table based on routing updates

    uint16_t prev_cost,new_cost;

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        for(int j=0; j<no_of_updates; j++)
        {
            if(memcmp(routing_table[i].router_id,routing_update[j].router_id,2) == 0)
            {
                prev_cost = routing_table[i].cost[0] * 256 + routing_table[i].cost[1];
                new_cost = routing_update[j].cost[0] * 256 + routing_update[j].cost[1];

                if(new_cost != 65535)
                {
                    new_cost = new_cost + cost_to_neighbour;
                }

                if(new_cost<prev_cost)
                {
                    uint16_t id;
                    memcpy(&id,routing_table[i].router_id,2);
                    printf("Updated new cost:%d to neighbour:%d\n",new_cost,ntohs(id));
                    memcpy(routing_table[i].next_hop_id,&neighbour_id,2);
                    new_cost = htons(new_cost);
                    memcpy(routing_table[i].cost,&new_cost,2);
                }
            }

        }
    }

    printf("Routing Table Updated\n");
}

