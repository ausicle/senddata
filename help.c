#include <stdio.h>

void
print_help(void)
{
	puts("\
Usage:\n\
	senddata receive\n\
	senddata send <ip_addr>\n\
\n\
Arguments:\n\
	-a <address>  Bind/Connect to specific address\n\
	-p <port>     Bind/Connect to specific port, defaults to 43337\n\
	-f <filename> Send file with filename, or write received data to filename\n\
	-e            Send/Receive data encrypted. Warning! Can only be used when\n\
	              the sender and the recipient use file send + file receive method\n\
	              otherwise the program will have an unexpected behavior.\
");
}
