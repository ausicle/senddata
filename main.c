#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "helper/help.h"
#include "helper/networking.h"

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 43337
#endif

int verbosity = 0;

FILE *fptr;
ssize_t bytes_received, bytes_read = 0;
int sockfd, recvfd;
int i;
struct sockaddr_in addr_in;

enum {
	SERVER,
	CLIENT
} role;
enum {
	STDIN,
	READ_FILE
} input_mode;
enum {
	ECHO,
	SAVE_FILE
} output_mode;

int
main(int argc, char **argv)
{
	in_addr_t addr = INADDR_ANY;
	in_port_t port = DEFAULT_PORT;
	socklen_t addrlen;
	char ch, opt, buf[1024] = {0};
	char *filename, *addrstr = "0";
	ssize_t buflen = sizeof(buf);

	/* Set mode and options */
	switch (argv[1][0]) {
		case 'r':
			role = SERVER;
			break;
		case 's':
			role = CLIENT;
			if (argv[2][0] == '-') {
				print_help();
				exit(EXIT_FAILURE);
			}
			addrstr = argv[2];
			++argv;
			--argc;
			break;
		default:
			print_help();
			exit(EXIT_FAILURE);
	}
	++argv;
	--argc;
	while ((opt = getopt(argc, argv, "?::p:f:b:a:")) >= 0) {
		switch (opt) {
			case '?':
				print_help();
				break;
			case 'a':
				addrstr = optarg;
				break;
			case 'p':
				port = strtoul(optarg, NULL, 0);
				break;
			case 'f':
				input_mode = READ_FILE;
				output_mode = SAVE_FILE;
				filename = optarg;
				break;
			case 'b':
				strcpy(buf, optarg);
				break;
			default:
				output_mode = ECHO;
		}
	}

	/* Initialize socket */
	addr_in.sin_family = AF_INET;
	if (addrstr[0] != '0')
	if (inet_pton(AF_INET, addrstr, &addr_in.sin_addr) <= 0) {
		perror("Invalid address");
		exit(EXIT_FAILURE);
	}
	addr_in.sin_port = htons(port);
	addrlen = sizeof(addr_in);

	if ((sockfd = initalize_socket(addr, port)) < 0) {
		perror("cannot create socket");
		return 0;
	}
	switch (role) {
		case SERVER:
			if (bind(sockfd, (struct sockaddr*) &addr_in, addrlen) < 0) {
				perror("cannot not bind socket");
				goto close_socket;
			}
			if (listen(sockfd, 3) < 0) {
				perror("cannot listen socket");
				goto close_socket;
			}
			printf("SERVER is RUNNING\nADDR: %X\nPORT: %u\n", addr_in.sin_addr.s_addr, port);
			if ((recvfd = accept(sockfd, (struct sockaddr*) &addr_in, &addrlen)) < 0) {
				perror("cannot accept incoming message");
				goto close_recv_socket;
			}
			/* Save incoming data */
			switch (output_mode) {
				case ECHO:
					puts("------------------------");
					while (1) {
						bytes_read = read(recvfd, &ch, 1);
						if (bytes_read < 0) {
							perror("error read socket");
							goto close_recv_socket;
						} else if (bytes_read == 0) {
							puts("Connection closed by peer");
							break;
						} else {
							if (ch == EOF)
								goto close_recv_socket;
							putchar(ch);
						}
					}
					break;
				case SAVE_FILE:
					fptr = fopen(filename, "w+");
					if (fptr == NULL) {
						perror("cannot read file descriptor");
						break;
					}
					if (write_socket_to_file(recvfd, fptr) < 0) {
						perror("cannot write socket to file");
					}
					fclose(fptr);
					break;
			}
			shutdown(recvfd, SHUT_RD);
			goto close_recv_socket;
		case CLIENT:
			if (connect(sockfd, (struct sockaddr*) &addr_in, addrlen)) {
				perror("cannot connect to address error");
				exit(3);
			}
			switch (input_mode) {
			case STDIN:
				while (1) {
					ch = getchar();
					buf[i++] = ch;
					switch (ch) {
						case '\n':
							send(sockfd, buf, i, 0);
							i = 0;
							break;
						case EOF:
							send(sockfd, buf, i, 0);
							i = 0;
							goto close_recv_socket;
					}
				}
				break;
			case READ_FILE:
				fptr = fopen(filename, "r");
				if (fptr == NULL) {
					perror("cannot read file descriptor");
					break;
				}
				fread(buf, 1, buflen, fptr);
				printf("Bytes send: %zd\n", send(sockfd, buf, buflen, 0));
				fclose(fptr);
				break;
			}
			shutdown(recvfd, SHUT_WR);
			goto close_socket;
	}

close_recv_socket:
	close(recvfd);
close_socket:
	close(sockfd);

	return 0;
}
