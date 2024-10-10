#ifndef HELPER_NETWORKING_H
#define HELPER_NETWORKING_H
#include <netinet/in.h>

/**
 * Initialize socket with SO_REUSEADDR and SO_REUSEPORT
 * @return socket file descriptor
 */
int initialize_socket(void);

/**
 * Resolve address from hostname
 * @params[in] addr_str string of address
 * @params[in,out] addr_in structure sockaddr_in to be output
 * @return 0 on succeed, -1 on error
 */
int resolve_addr(char *addr_str, struct sockaddr_in *addr_in);

/**
 * Initialize addr_in with address and port
 * @params[out] addr_in pointer to struct addr_in
 * @params[in] addr_str string of address i.e. "127.0.0.1"
 * @params[in] port i.e. 43337
 * @return sockaddr_in with port and address setted
 */
int initialize_addr_in(struct sockaddr_in *addr_in, char *addr_str,
                       in_port_t port);

/**
 * Bind, listen and accept sockfd.
 * @params[in] sockfd socket to bind
 * @params[in] addr_in address to bind
 * @return server file descriptor
 */
int start_server(int sockfd, struct sockaddr_in addr_in);

/**
 * Send file with filename
 * @params[in] filename name of file to send
 * @params[in] sockfd target socket file descriptor
 * @return 0 on succeed, -1 on error
 */
int sendfile_name(const char *filename, int sockfd);

/**
 * Write socket data to a file
 * @params[in] sockfd socket to read from
 * @params[in] filefd file to write to
 * @return total bytes received from sockfd
 */
int write_socket_to_file(int sockfd, int filefd);

#endif
