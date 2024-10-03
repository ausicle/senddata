#include "args.h"
#include "encryption.h"
#include "networking.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 43337
#endif

#define ENCRYPTED_FILE_EXT ".enc"
int filefd, sockfd, recvfd;
int bytes_received;
int status;
char address[INET_ADDRSTRLEN];
struct sockaddr_in addr_in;
unsigned char key[SHA256_DIGEST_LENGTH];
char filename_encrypted[256];
char buf[1024];
unsigned char iv[16];

int
main(int argc, char **argv)
{
	/* Parse arguments */
	struct u_option o = {.net = {.addr = "0", .port = DEFAULT_PORT}};
	parse_args(argc, argv, &o);
	if (o.enc.algo != NONE) {
		get_key(key);
		o.enc.key = key;
	}
	if (o.enc.algo == AES256) {
		if (RAND_bytes(iv, sizeof(iv)) != 1) {
			return -1;
		}
		strncpy(filename_encrypted, o.filename, sizeof(filename_encrypted));
		strncat(filename_encrypted, ENCRYPTED_FILE_EXT,
		        sizeof(filename_encrypted) - sizeof(ENCRYPTED_FILE_EXT));
	}

	/* Initialize socket */
	sockfd = initialize_socket();
	addr_in = initialize_addr_in(o.net.addr, o.net.port);
	if (sockfd < 0) {
		perror("cannot create socket");
		return 0;
	}

	/* Select SEND or RECEIVE */
	switch (o.action) {
	case SEND:
		/* Connect to server */
		if (connect(sockfd, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
			close(sockfd);
			perror("cannot connect to address");
			return 2;
		}
		if (inet_ntop(AF_INET, &addr_in.sin_addr, address, INET_ADDRSTRLEN) ==
		    NULL) {
			perror("cannot get address");
			return -1;
		}
		printf("Connected to address: %s\n", address);
		switch (o.input) {
		case STDIN:
			while (fgets(buf, sizeof(buf), stdin) != NULL) {
				send(sockfd, buf, strlen(buf), 0);
			}
			break;
		case READ_FILE:
			switch (o.enc.algo) {
			case AES256:
				file_encrypt(o.enc.key, o.filename, filename_encrypted);
				if (sendfile_name(filename_encrypted, sockfd) < 0) {
					perror("file cannot be sent");
				}
				break;
			default:
				if (sendfile_name(o.filename, sockfd) < 0) {
					perror("file cannot be sent");
				}
				break;
			}
			break;
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
		switch (o.output) {
		case STDOUT:
			puts("------------------------");
			while (1) {
				bytes_received = recv(recvfd, buf, sizeof(buf), 0);
				if (bytes_received < 0) {
					perror("could not write socket to stdout");
					return -1;
				} else if (bytes_received == 0) {
					puts("Connection closed by peer");
					break;
				}
				write(STDOUT_FILENO, buf, bytes_received);
			}
			break;
		case SAVE_FILE:
			if (o.enc.algo == NONE) {
				filefd = open(o.filename, O_CREAT | O_WRONLY | O_TRUNC,
				              S_IRUSR | S_IWUSR);
			} else {
				filefd = open(filename_encrypted, O_CREAT | O_WRONLY | O_TRUNC,
				              S_IRUSR | S_IWUSR);
			}
			if (filefd < 0) {
				perror("cannot open file");
				break;
			}
			if (write_socket_to_file(recvfd, filefd) < 0) {
				perror("cannot write socket to file");
			}
			close(filefd);
			if (o.enc.algo == AES256) {
				file_decrypt(o.enc.key, filename_encrypted, o.filename);
			}
			break;
		}
		shutdown(recvfd, SHUT_RD);
		close(recvfd);
		break;
	}
	close(sockfd);
	return 0;
}
