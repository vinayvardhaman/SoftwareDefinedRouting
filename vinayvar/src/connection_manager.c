/**
 * @connection_manager
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */

#include <sys/select.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "time.h"
#include "sys/time.h"

fd_set master_list, watch_list;
int head_fd;

void main_loop()
{
    int selret, sock_index, fdaccept;

    while(TRUE){

        if(init_received && !router_port_initialised)
        {
            initialise_router_port();
            router_port_initialised = TRUE;
        }
        watch_list = master_list;

        gettimeofday(&cst1, NULL);

        selret = select(head_fd+1, &watch_list, NULL, NULL, &tv);

        if(selret < 0)
            ERROR("select failed.");

        if(selret == 0)
        {
            // send updates to the neighbouring routers
            send_routing_updates();

            tv.tv_sec = UPDATE_INTERVAL;
            tv.tv_usec = 0;
            continue;
        }

        /* Loop through file descriptors to check which ones are ready */
        for(sock_index=0; sock_index<=head_fd; sock_index+=1){

            if(FD_ISSET(sock_index, &watch_list)){

                /* ssh-copy-id user@123.45.56.78control_socket */
                if(sock_index == control_socket){
                    fdaccept = new_control_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                /* router_socket */
                else if(sock_index == router_socket)
                {
                    //call handler that will call recvfrom() .....

                    receive_routing_updates(sock_index);


                }

                /* data_socket */
                else if(sock_index == data_socket){
                    //new_data_conn(sock_index);
                }

                /* Existing connection */
                else{
                    if(isControl(sock_index)){
                        if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                    }
                    //else if isData(sock_index);
                    else ERROR("Unknown socket index");
                }
            }
        }

        gettimeofday(&cst2,NULL);

 /*       if((tv.tv_sec == 0) && (tv.tv_usec == 0))
        {
            tv.tv_sec = UPDATE_INTERVAL;
            tv.tv_usec = 0;
        }*/

    }

}

void init()
{
    control_socket = create_control_sock();

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    tv.tv_sec = 1000;
    tv.tv_usec = 0;

    main_loop();
}

void initialise_router_port()
{
    router_socket = create_router_sock();


    /* Register the router socket */
    FD_SET(router_socket, &master_list);

    if(router_socket>head_fd)
    head_fd = router_socket;
}