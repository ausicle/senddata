#ifndef HELPER_NETWORKING_H
#define HELPER_NETWORKING_H
#include <netinet/in.h>

/**
 * Write socket data to a file
 * @params[in] socket file descriptor
 * @params[in] file descriptor
 * @return total bytes received from sockfd
 */
int write_socket_to_file(int sockfd, int filefd);

/**
 * Initialize socket with SO_REUSEADDR and SO_REUSEPORT
 * @return socket file descriptor
 */
int initialize_socket(void);

/**
 * Initialize sockaddr_in with address and port
 * @params[in] addr_str string of address i.e. "127.0.0.1"
 * @params[in] port i.e. 43337
 * @return sockaddr_in structure with port and address setted
 */
struct sockaddr_in
initialize_addr_in(char *addr_str, in_port_t port);

/**
 * Bind, listen and accept sockfd.
 * @params[in] sockfd socket file descriptor
 * @params[in] address of socket
 */
int start_server(int sockfd, struct sockaddr_in addr_in);

/**
 * Read stdin until EOF
 * @params[in] sockfd socket to send stdin
 * @return 0 on succeed
 */
int send_stdin(int sockfd);

/**
 * Send file with filename
 * @params[in] filename name of file to send
 * @params[in] sockfd target socket file descriptor
 * @return 0 on succeed, -1 on error
 */
int sendfile_name(const char *filename, int sockfd);

#endif
