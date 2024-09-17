#ifndef ARGS_H
#define ARGS_H

/**
 * Set verbosity to >0 for output verbosely
 */
extern int verbosity;

/**
 * General address and port
 */
struct netconfig_in {
	char *address;
	unsigned long port;
};

enum action_t { SEND, RECEIVE } action;
enum input_t { STDIN, READ_FILE } input;
enum output_t { ECHO, SAVE_FILE } output;

typedef struct {
	enum action_t action;
	enum input_t input;
	enum output_t output;
	char *filename;
	struct netconfig_in network;
} args_t;

int parse_args(int argc, char **argv, args_t *args);

#endif
