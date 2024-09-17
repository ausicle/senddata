#include "args.h"
#include "help.h"
#include <stdlib.h>
#include <unistd.h>

int verbosity = 0;

int
parse_args(int argc, char **argv, args_t *args)
{
	int opt;
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
	args->output = ECHO;
	while ((opt = getopt(argc, argv, "?::v::p:f:a:")) >= 0) {
		switch (opt) {
		case '?':
			print_help();
			exit(0);
			break;
		case 'a':
			args->network.address = optarg;
			break;
		case 'v':
			verbosity = 1;
			break;
		case 'p':
			args->network.port = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			args->filename = optarg;
			args->input = READ_FILE;
			args->output = SAVE_FILE;
			break;
		}
	}
	return 0;
}
