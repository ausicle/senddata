#include <stdio.h>

void
print_help(void)
{
	fprintf(stderr, "\
Usage:\n\
	senddata receive\n\
	senddata send <ip_addr>\n\
	[-v verbosity] [-p port] [-f file]\n\
");
}
