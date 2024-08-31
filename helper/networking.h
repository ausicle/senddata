#ifndef HELPER_NETWORKING_H
#define HELPER_NETWORKING_H
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * Write socket data to a file
 * @params[in] socket file descriptor
 * @params[out] file descriptor
*/
int
write_socket_to_file(int sockfd, FILE *fd)
{
	char buf[1024] = { 0 };
	ssize_t bufsize = sizeof(buf);
	int socklen;
	ssize_t bytes_received, bytes_read = 0;
	if (sockfd < 0) {
		perror("socket invalid");
		return -1;
	}
	if (fd < 0) {
		perror("file descriptor invalid");
		return -1;
	}
	ioctl(sockfd, FIONREAD, &socklen);
	while (bytes_read < socklen) {
		bytes_received = recv(sockfd, buf, bufsize, 0);
		if (bytes_received == 0) {
			puts("Connection closed");
			break;
		} else if (bytes_received < 0) {
			perror("error read from socket");
			break;
		} else {
			bytes_read += bytes_received;
		}
	}
	fputs(buf, fd);
	return bytes_read;
}

int
initalize_socket(in_addr_t addr, in_port_t port)
{
	int sockfd, opt = 1;
	/* Initalize socket file descriptor */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create socket");
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("cannot set socket options SO_REUSEADDR");
		close(sockfd);
		return -1;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
		perror("cannot set socket options SO_REUSEPORT");
		close(sockfd);
		return -1;
	}
#endif
	return sockfd;
}

#endif
