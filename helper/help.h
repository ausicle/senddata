#ifndef HELPER_HELP_H
#define HELPER_HELP_H
#include <stdio.h>

void
print_help(void)
{
	fprintf(stderr, "\
Usage:\n\
	senddata receive [-p port] [-f file]\n\
	senddata send <ip_addr> [-p port] [-f file]\n\
");
}
#endif
