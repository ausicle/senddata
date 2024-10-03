#include "args.h"
#include "help.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

int
parse_args(int argc, char **argv, struct u_option *o)
{
	int opt;
	/* Set mode and options */
	if (argc < 2) {
		print_help();
		exit(EXIT_FAILURE);
	}
	switch (argv[1][0]) {
	case 'r':
		o->action = RECEIVE;
		break;
	case 's':
		o->action = SEND;
		if (argc < 3 || argv[2][0] == '-') {
			print_help();
			exit(EXIT_FAILURE);
		}
		o->net.addr = argv[2];
		++argv;
		--argc;
		break;
	default:
		print_help();
		exit(EXIT_FAILURE);
	}
	++argv;
	--argc;
	o->output = STDOUT;
	while ((opt = getopt(argc, argv, "?::e::p:f:a:")) >= 0) {
		switch (opt) {
		case '?':
			print_help();
			exit(0);
			break;
		case 'a':
			o->net.addr = optarg;
			break;
		case 'e':
			o->enc.algo = AES256;
			break;
		case 'p':
			o->net.port = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			o->filename = optarg;
			o->input = READ_FILE;
			o->output = SAVE_FILE;
			break;
		}
	}
	return 0;
}
