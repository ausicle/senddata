#ifndef HELPER_ARGS_H
#define HELPER_ARGS_H
#include "help.h"
#include <stdlib.h>
#include <unistd.h>

struct netconfig_in {
	char *address;
	unsigned long port;
};

typedef struct {
	enum action_t { SEND, RECEIVE } action;
	enum input_t { STDIN, READ_FILE } input;
	enum output_t { ECHO, SAVE_FILE } output;
	char *filename;
	struct netconfig_in network;
} args_t;

int
parse_args(int argc, char **argv, args_t *args)
{
	char opt;
	/* Set mode and options */
	if (argc < 2) {
		print_help();
		exit(EXIT_FAILURE);
	}
	switch (argv[1][0]) {
	case 'r':
		args->action = RECEIVE;
		break;
	case 's':
		args->action = SEND;
		if (argc < 3 || argv[2][0] == '-') {
			print_help();
			exit(EXIT_FAILURE);
		}
		args->network.address = argv[2];
		++argv;
		--argc;
		break;
	default:
		print_help();
		exit(EXIT_FAILURE);
	}
	++argv;
	--argc;
	while ((opt = getopt(argc, argv, "?::p:f:a:")) >= 0) {
		switch (opt) {
		case '?':
			print_help();
			exit(0);
			break;
		case 'a':
			args->network.address = optarg;
			break;
		case 'p':
			args->network.port = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			args->filename = optarg;
			args->input = READ_FILE;
			args->output = SAVE_FILE;
			break;
		default:
			args->output = ECHO;
		}
	}
	return 0;
}

#endif
