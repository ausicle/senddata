#include "args.h"        // for action_t, input_t, netconfig_in, out...
#include "networking.h"  // for write_socket_to_file, initialize_add...
#include <netinet/in.h>         // for sockaddr_in
#include <stdio.h>              // for perror, puts
#include <sys/_types/_s_ifmt.h> // for S_IRUSR, S_IWUSR
#include <sys/fcntl.h>          // for open, O_CREAT, O_TRUNC, O_WRONLY
#include <sys/socket.h>         // for shutdown, connect, SHUT_RD, SHUT_WR
#include <unistd.h>             // for close, STDOUT_FILENO

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 43337
#endif

int fd, sockfd, recvfd;
int status;
struct sockaddr_in addr_in;

int
main(int argc, char **argv)
{
	args_t args = {.network = {.address = "0", .port = DEFAULT_PORT}};
	parse_args(argc, argv, &args);

	/* Initialize socket */
	addr_in = initialize_addr_in(args.network.address, args.network.port);

	if ((sockfd = initialize_socket()) < 0) {
		perror("cannot create socket");
		return 0;
	}
	switch (args.action) {
	case SEND:
		if (connect(sockfd, (struct sockaddr *)&addr_in,
		            sizeof(addr_in))) {
			perror("cannot connect to address");
			goto close_socket;
		}
		switch (args.input) {
		case STDIN:
			if (send_stdin(sockfd) < 0)
				perror("cannot send stdin");
			break;
		case READ_FILE:
			if (sendfile_name(args.filename, sockfd) < 0)
				perror("file cannot be sent");
			break;
		}
		shutdown(recvfd, SHUT_WR);
		goto close_socket;
	case RECEIVE:
		if ((recvfd = start_server(sockfd, addr_in)) < 0)
			goto close_socket;
		/* Save incoming data */
		switch (args.output) {
		case ECHO:
			puts("------------------------");
			if (write_socket_to_file(recvfd, STDOUT_FILENO) < 0)
				perror("could not write socket to stdout");
			else
				puts("Connection closed by peer");
			goto close_recv_socket;
		case SAVE_FILE:
			fd = open(args.filename, O_CREAT | O_WRONLY | O_TRUNC,
			          S_IRUSR | S_IWUSR);
			if (fd < 0) {
				perror("cannot open file");
				break;
			}
			if (write_socket_to_file(recvfd, fd) < 0) {
				perror("cannot write socket to file");
			}
			close(fd);
			break;
		}
		shutdown(recvfd, SHUT_RD);
		goto close_recv_socket;
	}

close_recv_socket:
	close(recvfd);
close_socket:
	close(sockfd);

	return 0;
}
