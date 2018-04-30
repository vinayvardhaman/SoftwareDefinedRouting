//
// Created by yaniv on 11/25/17.
//

#include <string.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include <netinet/in.h>
#include "sys/time.h"

uint16_t max = 65535;

void init_response(int socket_index, char* cntrl_payload)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    cntrl_response_header = create_response_header(socket_index, 1, 0, 0);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    /* Payload is empty*/
    sendALL(socket_index, cntrl_response, response_len);

    free(cntrl_response);

    /* Copy the Routers Info*/

    char temp[2];
    memcpy(temp,cntrl_payload,2);


    NO_OF_ROUTERS = temp[0]*256 + temp[1];

    memcpy(temp,cntrl_payload+2,2);
    UPDATE_INTERVAL = temp[0]*256 + temp[1];

    gettimeofday(&cst1, NULL);

    printf("In init Response, no of routers:%d\n",NO_OF_ROUTERS);
    printf("In init Response, Update interval:%d\n",UPDATE_INTERVAL);

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        memcpy(router_info[i].router_id, cntrl_payload+4+i*12, 2);
        memcpy(router_info[i].router_port1, cntrl_payload+4+i*12+2, 2);
        memcpy(router_info[i].router_port2, cntrl_payload+4+i*12+4, 2);
        memcpy(router_info[i].cost, cntrl_payload+4+i*12+6, 2);
        memcpy(router_info[i].init_cost, cntrl_payload+4+i*12+6, 2);
        memcpy(router_info[i].ip_addr, cntrl_payload+4+i*12+8, 4);

    }

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        memcpy(routing_table[i].router_id, router_info[i].router_id, 2);
        memset(routing_table[i].padding, 0, 2);
        memcpy(routing_table[i].cost, router_info[i].cost, 2);
        memcpy(routing_table[i].init_cost, router_info[i].init_cost,2);
        memcpy(routing_table[i].router_port, router_info[i].router_port1,2);
        memcpy(routing_table[i].data_port, router_info[i].router_port2,2);
        memcpy(routing_table[i].ip_addr, router_info[i].ip_addr,4);

        uint16_t cost = router_info[i].cost[0]*256 + router_info[i].cost[1];
        if(cost == max)
        {
            memcpy(routing_table[i].next_hop_id, router_info[i].cost, 2);
            routing_table[i].isneighbour = FALSE;
        }
        else
        {
            memcpy(routing_table[i].next_hop_id, routing_table[i].router_id, 2);
            if(cost == 0)
            {
                memcpy(&ROUTER_PORT, router_info[i].router_port1,2);
                ROUTER_PORT = ntohs(ROUTER_PORT);

                memcpy(&DATA_PORT, router_info[i].router_port2,2);
                DATA_PORT = ntohs(DATA_PORT);

                memcpy(&IP_ADDRESS, router_info[i].ip_addr,4);
                IP_ADDRESS = ntohl(IP_ADDRESS);
                routing_table[i].isneighbour = FALSE;
                printf("In init Response, Router Port:%d\n",ROUTER_PORT);
            }
            else
            {
                routing_table[i].isneighbour = TRUE;
            }
        }



    }
    // consider clearing remaining router info

    init_received = TRUE;

    tv.tv_sec = UPDATE_INTERVAL;
    tv.tv_usec = 0;

}

void routing_table_response(int socket_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    payload_len = NO_OF_ROUTERS* 8;
    cntrl_response_payload = (char*)malloc(payload_len);

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        memcpy(cntrl_response_payload+i*8, routing_table[i].router_id, 2);
        memcpy(cntrl_response_payload+i*8+2, routing_table[i].padding, 2);
        memcpy(cntrl_response_payload+i*8+4, routing_table[i].next_hop_id, 2);
        memcpy(cntrl_response_payload+i*8+6, routing_table[i].cost, 2);

    }


    cntrl_response_header = create_response_header(socket_index, 2, 0, payload_len);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);

    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    /* Copy Payload */
    memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
    free(cntrl_response_payload);

    sendALL(socket_index, cntrl_response, response_len);

    free(cntrl_response);
}

void update_response(int socket_index, char* cntrl_payload)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;
    uint16_t id;

    cntrl_response_header = create_response_header(socket_index, 3, 0, 0);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    /* Payload is empty*/
    sendALL(socket_index, cntrl_response, response_len);

    memcpy(&id,cntrl_payload,2);

    for(int i=0; i<NO_OF_ROUTERS; i++)
    {
        if(memcmp(&id,routing_table[i].router_id,2)==0)
        {
            memcpy(routing_table[i].cost,cntrl_payload+2,2);
            memcpy(routing_table[i].init_cost,cntrl_payload+2,2);
        }
    }

    free(cntrl_response);
}

void crash_response(int socket_index)
{
    uint16_t payload_len, response_len;
    char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

    cntrl_response_header = create_response_header(socket_index, 4, 0, 0);

    response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
    cntrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
    free(cntrl_response_header);

    /* Payload is empty*/
    sendALL(socket_index, cntrl_response, response_len);

    free(cntrl_response);
}
