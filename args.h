#ifndef ARGS_H
#define ARGS_H

/**
 * Set verbosity to >0 for verbose output
 */
extern int verbosity;

/**
 * General address and port
 */
struct netconfig_in {
	char *address;
	unsigned long port;
};

enum action_t { SEND, RECEIVE };
enum input_t { STDIN, READ_FILE };
enum output_t { STDOUT, SAVE_FILE };

typedef struct {
	enum action_t action;
	enum input_t input;
	enum output_t output;
	char *filename;
	struct netconfig_in network;
} args_t;

/**
 * Parse args into args
 * @param[in] argc
 * @param[in] argv
 * @param[out] args variable stored parsed data
 * @return 0 on succeed
 */
int parse_args(int argc, char **argv, args_t *args);

#endif
