#ifndef HELPER_NETWORKING_H
#define HELPER_NETWORKING_H
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Write socket data to a file
 * @params[in] socket file descriptor
 * @params[out] file descriptor
 */
int
write_socket_to_file(int sockfd, int fd)
{
	char buf[1024] = {0};
	ssize_t bufsize = sizeof(buf);
	ssize_t bytes_received, bytes_read = 0;
	if (sockfd < 0) {
		errno = EBADF;
		return -1;
	}
	if (fd < 0) {
		errno = EBADF;
		return -1;
	}
	while (1) {
		bytes_received = recv(sockfd, buf, bufsize, 0);
		if (bytes_received < 0) {
			return -1;
		} else if (bytes_received == 0) {
			break;
		} else if (buf[bytes_received - 1] == EOF) {
			break;
		}
		write(fd, buf, bytes_received);
		bytes_read += bytes_received;
	}
	return bytes_read;
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
	int i = 0;
	while (1) {
		buf[i++] = getchar();
		switch (buf[i - 1]) {
		case '\n':
			send(sockfd, buf, i, 0);
			buf[i++] = '\0';
			i = 0;
			break;
		case EOF:
			send(sockfd, buf, i, 0);
			buf[i++] = '\0';
			i = 0;
			return 0;
		}
	}
	return 0;
}

int
send_file(const char *filename, int sockfd)
{
	int fd;
	struct stat file_stat;
	off_t len;
	struct sf_hdtr hdtr = {NULL, 0, NULL, 0};
	if ((fd = open(filename, O_RDONLY)) < 0) {
		return -1;
	}
	if (fstat(fd, &file_stat) < 0) {
		return -1;
	}
	len = file_stat.st_size;
	while (1) {
		if (sendfile(fd, sockfd, 0, &len, &hdtr, 0) < 0) {
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
	/*
	char buf[1024];
	ssize_t buflen = sizeof(buf);
	int bytes_read;
	if (fd == NULL) {
	        perror("cannot read file descriptor");
	        return -1;
	}
	while (1) {
	        if ((bytes_read = fread(buf, 1, buflen, fd)) > 0)
	                printf("Bytes send: %zd\n",
	                       send(sockfd, buf, buflen, 0));
	}
	return 0;
	*/
}

#endif
