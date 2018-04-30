#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_

int control_socket, router_socket, data_socket;

void init();
void initialise_router_port();

#endif