#ifndef HELPER_NETWORKING_H
#define HELPER_NETWORKING_H
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/sendfile.h>
#endif

/**
 * Write socket data to a file
 * @params[in] socket file descriptor
 * @params[out] file descriptor
 */
int
write_socket_to_file(int sockfd, int fd)
{
	int buf[1024] = {0};
	ssize_t bufsize = sizeof(buf);
	size_t bytes_received = 0;
	ssize_t bytes_received_total = 0;
	if (fd < 0 || sockfd < 0) {
		errno = EBADF;
		return -1;
	}
	while (1) {
		bytes_received = recv(sockfd, buf, bufsize, 0);
		if (bytes_received < 0) {
			return -1;
		} else if (bytes_received == 0) {
			break;
		}
		write(fd, buf, bytes_received);
		bytes_received_total += bytes_received;
		if (verbosity)
			printf("Got %zu bytes, total %zd bytes\n",
			       bytes_received, bytes_received_total);
	}
	return bytes_received_total;
}

/**
 * Initialize socket with SO_REUSEADDR and SO_REUSEPORT
 * @return socket file descriptor
 */
int
initialize_socket(void)
{
	int sockfd, opt = 1;
	/* Initialize socket file descriptor */
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
	    0) {
		close(sockfd);
		return -1;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) <
	    0) {
		close(sockfd);
		return -1;
	}
#endif
	return sockfd;
}

struct sockaddr_in
initialize_addr_in(char *addr_str, in_port_t port)
{
	struct sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	if (addr_str[0] != '0') {
		if (inet_pton(AF_INET, addr_str, &addr_in.sin_addr) <= 0) {
			perror("Invalid address");
			exit(EXIT_FAILURE);
		}
	} else
		addr_in.sin_addr.s_addr = 0;
	addr_in.sin_port = htons(port);
	return addr_in;
}

/* void
 * print_addrs(void)
 * {
 *         struct ifaddrs *addrs, *tmpaddrs;
 *         getifaddrs(&addrs);
 *         tmpaddrs = addrs;
 *         while (tmpaddrs)
 *         {
 *                 if (tmpaddrs->ifa_addr &&
 *                     tmpaddrs->ifa_addr->sa_family == PF_INET)
 *                         printf("%s: %d\n", tmpaddrs->ifa_name,
 * tmpaddrs->ifa_addr); tmpaddrs = tmpaddrs->ifa_next;
 *         }
 *         freeifaddrs(addrs);
 * } */

int
start_server(int sockfd, struct sockaddr_in addr_in)
{
	int recvfd;
	socklen_t addrlen = sizeof(addr_in);
	if (bind(sockfd, (struct sockaddr *)&addr_in, addrlen) < 0) {
		return -1;
	}
	if (listen(sockfd, 3) < 0) {
		return -1;
	}
	printf("SERVER is RUNNING\nADDR: %X\nPORT: %u\n",
	       addr_in.sin_addr.s_addr, ntohs(addr_in.sin_port));
	if ((recvfd = accept(sockfd, (struct sockaddr *)&addr_in, &addrlen)) <
	    0) {
		return -1;
	}
	return recvfd;
}

int
send_stdin(int sockfd)
{
	char buf[1024];
	while (fgets(buf, sizeof(buf), stdin) != NULL)
		send(sockfd, buf, strlen(buf), 0);
	return 0;
}

int
send_file(const char *filename, int sockfd)
{
	int fd;
	off_t len;
	struct stat file_stat;
#if defined(__APPLE__) || defined(__FreeBSD__)
	struct sf_hdtr hdtr = {NULL, 0, NULL, 0};
	int bytes_sent = 0;
#elif defined(__linux__)
	ssize_t bytes_sent = 0;
#elif defined(__unix__)
#endif
	if ((fd = open(filename, O_RDONLY)) < 0) {
		return -1;
	}
	if (fstat(fd, &file_stat) < 0) {
		return -1;
	}
	len = file_stat.st_size;
	while (1) {
#if defined(__APPLE__) || defined(__FreeBSD__)
		bytes_sent = sendfile(fd, sockfd, 0, &len, &hdtr, 0);
#elif defined(__linux__)
		bytes_sent = sendfile(sockfd, fd, NULL, len);
#elif defined(__unix__)
#endif
		if (bytes_sent < 0) {
			close(fd);
			return -1;
		}
		if (errno == EAGAIN)
			continue;
		else
			break;
	}
	close(fd);
	return 0;
}

#endif
