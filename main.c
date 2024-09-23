#include "args.h"
#include "networking.h"
#include <netinet/in.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 43337
#endif

int filefd, sockfd, recvfd;
int status;
struct sockaddr_in addr_in;

int
main(int argc, char **argv)
{
	/* Parse arguments */
	args_t args = {.network = {.address = "0", .port = DEFAULT_PORT}};
	parse_args(argc, argv, &args);

	/* Initialize socket */
	sockfd = initialize_socket();
	addr_in = initialize_addr_in(args.network.address, args.network.port);
	if (sockfd < 0) {
		perror("cannot create socket");
		return 0;
	}

	/* Select SEND or RECEIVE */
	switch (args.action) {
	case SEND:
		/* Connect to server */
		if (connect(sockfd, (struct sockaddr *)&addr_in,
		            sizeof(addr_in)) < 0) {
			close(sockfd);
			perror("cannot connect to address");
			return 2;
		} else {
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
		}
		shutdown(sockfd, SHUT_WR);
		break;
	case RECEIVE:
		/* Start server */
		recvfd = start_server(sockfd, addr_in);
		if (recvfd < 0) {
			close(sockfd);
			perror("cannot start server");
			return 2;
		}
		/* Output mode */
		switch (args.output) {
		case STDOUT:
			puts("------------------------");
			if (write_socket_to_file(recvfd, STDOUT_FILENO) < 0)
				perror("could not write socket to stdout");
			else
				puts("Connection closed by peer");
			break;
		case SAVE_FILE:
			filefd =
			    open(args.filename, O_CREAT | O_WRONLY | O_TRUNC,
			         S_IRUSR | S_IWUSR);
			if (filefd < 0) {
				perror("cannot open file");
				break;
			}
			if (write_socket_to_file(recvfd, filefd) < 0)
				perror("cannot write socket to file");
			close(filefd);
			break;
		}
		shutdown(recvfd, SHUT_RD);
		close(recvfd);
		break;
	}
	close(sockfd);
	return 0;
}
