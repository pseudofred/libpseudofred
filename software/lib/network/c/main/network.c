#include "network.h"

#include <stdio.h>

#include "apr-1/apr.h"
#include "apr-1/apr_errno.h"
#include "apr-1/apr_network_io.h"

#define MAX_SERVER_CONNECTIONS 1
#define CLIENT_CONNECT_TIMEOUT_S 30

static apr_pool_t *mem_pool;

// #################################################################################################
void print_apr_error(const apr_status_t status) {
  char err_buf[256];
  apr_strerror(status, err_buf, sizeof(err_buf));
  printf("Error: %d, %s\n", status, err_buf);
}

// #################################################################################################
int net_initialise() {
  apr_status_t ret = apr_initialize();
  if (ret != APR_SUCCESS) {
    printf("Error initialising APR.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_pool_create(&mem_pool, NULL);
  if (ret != APR_SUCCESS) {
    printf("Error creating memory pool.\n");
    print_apr_error(ret);
    return 1;
  }
  return 0;
}

// #################################################################################################
void net_terminate() {
  apr_pool_destroy(mem_pool);
  apr_terminate();
}

// GENERAL
// #################################################################################################
int net_socket_send_data(struct net_socket *socket, const char *data,
                         const size_t data_size) {
  if (!socket->connected) {
    printf("Error socket is not connected to an endpoint.\n");
    return 1;
  }

  apr_size_t bytes_sent = data_size;

  apr_status_t ret = apr_socket_send(socket->socket, data, &bytes_sent);
  if (ret != APR_SUCCESS) {
    printf("Error sending data. Number of bytes sent: %zu\n", bytes_sent);
    print_apr_error(ret);
    return 1;
  }

  if (bytes_sent != data_size) {
    printf("Bytes sent != bytes to send: %zu != %zu\n", bytes_sent,
           data_size);
    return 1;
  }

  return 0;
}

// #################################################################################################
int net_socket_read_data(struct net_socket *socket, char *data,
                         const size_t data_size) {
  if (!socket->connected) {
    printf("Error socket is not connected to an endpoint.\n");
    return 1;
  }

  apr_size_t bytes_received = data_size;

  apr_status_t ret = apr_socket_recv(socket->socket, data, &bytes_received);
  if (ret != APR_SUCCESS) {
    printf("Error reading data. Number of bytes read: %zu\n", bytes_received);
    print_apr_error(ret);
    return 1;
  }

  if (bytes_received != data_size) {
    printf("Bytes received != bytes to read: %zu != %zu\n", bytes_received,
           data_size);
    return 1;
  }

  return 0;
}

// #################################################################################################
int net_close_socket(struct net_socket *socket) {
  apr_status_t ret = apr_socket_close(socket->socket);
  if (ret != APR_SUCCESS) {
    printf("Error closing socket.\n");
    print_apr_error(ret);
    return 1;
  }
  socket->connected = false;
  socket->initialised = false;
  return 0;
}

// SERVER
// #################################################################################################
int net_create_server(struct net_socket *server, const char *hostname,
                      int port) {
  if (server->initialised) {
    printf("Server is already initialised, please close it.\n");
    return 1;
  }

  apr_sockaddr_t *socket_addr;
  apr_socket_t *socket;

  apr_status_t ret = apr_sockaddr_info_get(&socket_addr, hostname, APR_INET,
                                           port, 0, mem_pool);
  if (ret != APR_SUCCESS) {
    printf("Error getting socket address information.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_create(&socket, socket_addr->family, SOCK_STREAM,
                          APR_PROTO_TCP, mem_pool);
  if (ret != APR_SUCCESS) {
    printf("Error creating socket.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
  if (ret == APR_SUCCESS)
    ret = apr_socket_timeout_set(socket, -1);
  if (ret == APR_SUCCESS)
    ret = apr_socket_opt_set(socket, APR_SO_REUSEADDR, 1);
  if (ret != APR_SUCCESS) {
    printf("Error setting socket options.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_bind(socket, socket_addr);
  if (ret != APR_SUCCESS) {
    printf("Error binding socket.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_listen(socket, MAX_SERVER_CONNECTIONS);
  if (ret != APR_SUCCESS) {
    printf("Error listening.\n");
    print_apr_error(ret);
    return 1;
  }

  server->initialised = true;
  server->socket = socket;
  return 0;
}

// #################################################################################################
int net_server_accept(struct net_socket *client, struct net_socket *server) {
  if (!server->initialised) {
    printf("Server is not initialised.\n");
    return 1;
  }

  if (server->connected) {
    printf("Server is already connected to another endpoint, please close the "
           "connection.\n");
    return 1;
  }

  apr_socket_t *socket;

  apr_status_t ret = apr_socket_accept(&socket, server->socket, mem_pool);
  if (ret != APR_SUCCESS) {
    printf("Error accepting connection.\n");
    print_apr_error(ret);
    return 1;
  }

  server->connected = true;
  client->initialised = true;
  client->connected = true;
  client->socket = socket;
  return 0;
}

// CLIENT
// #################################################################################################
int net_create_client(struct net_socket *client, const char *server_hostname,
                      const int server_port) {
  if (client->initialised) {
    printf("Client is already initialised.\n");
    return 1;
  }

  apr_sockaddr_t *socket_addr;
  apr_socket_t *socket;

  apr_status_t ret = apr_sockaddr_info_get(&socket_addr, server_hostname,
                                           APR_INET, server_port, 0, mem_pool);
  if (ret != APR_SUCCESS) {
    printf("Error getting socket address information.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_create(&socket, socket_addr->family, SOCK_STREAM,
                          APR_PROTO_TCP, mem_pool);
  if (ret != APR_SUCCESS) {
    printf("Error creating socket.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
  if (ret == APR_SUCCESS)
    ret = apr_socket_timeout_set(socket,
                                 (APR_USEC_PER_SEC * CLIENT_CONNECT_TIMEOUT_S));
  if (ret != APR_SUCCESS) {
    printf("Error setting socket options.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_connect(socket, socket_addr);
  if (ret != APR_SUCCESS) {
    printf("Error connecting to socket.\n");
    print_apr_error(ret);
    return 1;
  }

  ret = apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
  if (ret == APR_SUCCESS)
    ret = apr_socket_timeout_set(socket,
                                 (APR_USEC_PER_SEC * CLIENT_CONNECT_TIMEOUT_S));
  if (ret != APR_SUCCESS) {
    printf("Error setting socket options.\n");
    print_apr_error(ret);
    return 1;
  }

  client->initialised = true;
  client->connected = true;
  client->socket = socket;
  return 0;
}
