#ifndef NETWORKING_H
#define NETWORKING_H

#include "apr-1/apr_network_io.h"
#include <stdbool.h>

struct net_socket {
  // Status
  bool initialised;
  bool connected;

  // APR Structures
  apr_socket_t *socket;
};

int net_initialise();
void net_terminate();

int net_socket_send_data(struct net_socket *socket, const char *data,
                         const size_t data_size);
int net_socket_read_data(struct net_socket *socket, char *data,
                         const size_t data_size);
int net_close_socket(struct net_socket *socket);

int net_create_server(struct net_socket *server, const char *hostname,
                      int port);
int net_server_accept(struct net_socket *client, struct net_socket *server);

int net_create_client(struct net_socket *client, const char *server_hostname,
                      const int server_port);

#endif // NETWORKING_H
