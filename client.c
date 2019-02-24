#include <stdio.h>
#include <stdlib.h>

#include <string.h>


/* Main function */
int main(int argc, char *argv[])
{
	char cfg_file[] = "client.cfg";




	/* argument functionalities */
	if(argc > 1)
	{
		if(strcmp(argv[1],"-c") == 0)
		{
			if(argc != 3)
			{	
				printf("Use:\n");
				printf("\t%s -c <nom_arxiu>\n",argv[0]);
				exit(-1);
			}
			else
			{
				sprintf(cfg_file, argv[2]);
			}
		}
	}
	
	return 0;
}






