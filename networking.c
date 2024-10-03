#if defined(__linux__)
#define _POSIX_C_SOURCE 200112L
#include <sys/socket.h>
#else
#include <sys/socket.h>
#define _POSIX_C_SOURCE 200112L
#endif
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/sendfile.h>
#endif

int
initialize_socket(void)
{
	int sockfd, opt = 1;
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		close(sockfd);
		return -1;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
		close(sockfd);
		return -1;
	}
#endif
	return sockfd;
}

int
print_if_addrs(void)
{
	struct ifaddrs *ifaddr, *ifa;
	char ip_str[INET6_ADDRSTRLEN];

	if (getifaddrs(&ifaddr) == -1) {
		return -1;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
			inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
			printf("Interface: %s, Address: %s\n", ifa->ifa_name, ip_str);
		}
	}

	freeifaddrs(ifaddr);
	return 0;
}

int
resolve_addr(char *addr_str, struct sockaddr_in *addr_in)
{
	// Convert addr to network format
	if (inet_pton(AF_INET, addr_str, &addr_in->sin_addr) == 1) {
		return 0;
	}

	// Resolve hostname if above fails
	struct addrinfo hints = {0}, *res;
	hints.ai_family = addr_in->sin_family;
	hints.ai_socktype = SOCK_STREAM;
	int ret = getaddrinfo(addr_str, NULL, &hints, &res);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}
	struct sockaddr_in *resolved_addr = (struct sockaddr_in *)res->ai_addr;
	addr_in->sin_family = resolved_addr->sin_family;
	addr_in->sin_addr = resolved_addr->sin_addr;
	return 0;
}

struct sockaddr_in
initialize_addr_in(char *addr_str, in_port_t port)
{
	struct sockaddr_in addr_in;
	addr_in.sin_family = AF_INET;
	if (resolve_addr(addr_str, &addr_in) < 0) {
		exit(EXIT_FAILURE);
	}
	addr_in.sin_port = htons(port);
	return addr_in;
}

int
start_server(int sockfd, struct sockaddr_in addr_in)
{
	int recvfd;
	char address[INET_ADDRSTRLEN];
	char hostname[sysconf(_SC_HOST_NAME_MAX)];
	socklen_t addrlen = sizeof(addr_in);
	if (bind(sockfd, (struct sockaddr *)&addr_in, addrlen) < 0) {
		return -1;
	}
	if (listen(sockfd, 3) < 0) {
		return -1;
	}
	if (inet_ntop(AF_INET, &addr_in.sin_addr, address, INET_ADDRSTRLEN) ==
	    NULL) {
		return -1;
	}
	puts("SERVER is RUNNING");
	gethostname(hostname, sizeof(hostname));
	printf("HOSTNAME: %s\n", hostname);
	if (addr_in.sin_addr.s_addr == 0) {
		print_if_addrs();
	} else {
		printf("ADDR: %s\n", address);
	}
	printf("PORT: %u\n", ntohs(addr_in.sin_port));
	recvfd = accept(sockfd, (struct sockaddr *)&addr_in, &addrlen);
	if (recvfd < 0) {
		return -1;
	}
	return recvfd;
}

int
sendfile_name(const char *filename, int sockfd)
{
	int filefd;
	off_t filesize, total_bytes_sent = 0;
	struct stat file_stat;
#if defined(__APPLE__)
	int ret;
	struct sf_hdtr hdtr = {NULL, 0, NULL, 0};
	off_t sendlen;
#elif defined(__FreeBSD__)
	int ret;
	struct sf_hdtr hdtr = {NULL, 0, NULL, 0};
	off_t bytes_sent;
	off_t sendlen;
#elif defined(__linux__)
	ssize_t sendlen;
#endif
	filefd = open(filename, O_RDONLY);
	if (filefd < 0) {
		return -1;
	}
	if (fstat(filefd, &file_stat) < 0) {
		return -1;
	}
	filesize = file_stat.st_size;
	while (1) {
		sendlen = filesize - total_bytes_sent;
#if defined(__APPLE__)
		ret = sendfile(filefd, sockfd, total_bytes_sent, &sendlen, &hdtr, 0);
		if (ret < 0) {
			if (errno == EAGAIN) {
				continue;
			} else {
				return -1;
			}
		} else {
			break;
		}
		total_bytes_sent += sendlen;
#elif defined(__FreeBSD__)
		ret = sendfile(filefd, sockfd, total_bytes_sent, sendlen, &hdtr,
		               &bytes_sent, 0);
		if (ret < 0) {
			if (errno == EAGAIN) {
				continue;
			} else {
				return -1;
			}
		} else {
			break;
		}
		total_bytes_sent += bytes_sent;
#elif defined(__linux__)
		sendlen = sendfile(sockfd, filefd, &total_bytes_sent, sendlen);
		if (sendlen < 0) {
			if (errno == EAGAIN) {
				continue;
			} else {
				return -1;
			}
		} else {
			break;
		}
#endif
	}
	close(filefd);
	return 0;
}

/**
 * Write socket data to a file
 * @params[in] socket file descriptor
 * @params[out] file descriptor
 * @return bytes_received_total total bytes received
 */
int
write_socket_to_file(int sockfd, int fd)
{
	int buf[1024] = {0};
	ssize_t bufsize = sizeof(buf);
	ssize_t bytes_received = 0;
	size_t bytes_received_total = 0;
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
	}
	return bytes_received_total;
}
