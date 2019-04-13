#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <getopt.h>


int main(int argc, char **argv)
{
	char *cfg_file = "client.cfg";
	int debug = 0;
	int option;

	while((option = getopt(argc, argv, "dc:")) != -1){
		switch (option)
		{
			case 'c':
				cfg_file = optarg;
				break;
			case 'd':
				debug = 1;
				break;
			default:
				fprintf(stderr, "Argument incorrecte");
				exit(1);
		}

	}
	fprintf(stderr, "Cfg_file name: %s\n",cfg_file);
	fprintf(stderr, "Debug: %d\n", debug);
	

	return 0;
}
