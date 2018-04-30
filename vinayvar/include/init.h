//
// Created by yaniv on 11/25/17.
//

#ifndef INIT_H
#define INIT_H

void init_response(int socket_index, char* cntrl_payload);
void routing_table_response(int socket_index);
void update_response(int socket_index, char* cntrl_payload);
void crash_response(int socket);


#endif //INIT_H
