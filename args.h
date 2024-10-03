#ifndef ARGS_H
#define ARGS_H
#include <limits.h>

/**
 * General address and port
 */
struct netconfig_in {
	char *addr;
	unsigned long port;
};

enum action { SEND, RECEIVE };
enum input { STDIN, READ_FILE };
enum output { STDOUT, SAVE_FILE };

enum enc_algo { NONE, AES256 };

struct encryption {
	enum enc_algo algo;
	unsigned char *key;
};

struct u_option {
	enum action action;
	enum input input;
	enum output output;
	char *filename;
	struct encryption enc;
	struct netconfig_in net;
};

/**
 * Parse args into args
 * @param[in] argc
 * @param[in] argv
 * @param[out] args variable stored parsed data
 * @return 0 on succeed
 */
int parse_args(int argc, char **argv, struct u_option *o);

#endif
