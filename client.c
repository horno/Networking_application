#include <stdio.h>
#include <stdlib.h>

#include <string.h>


/* Main function */
int main(int argc, char *argv[])
{
	char cfg_file[] = "client.cfg";

	FILE *fpointer;
	char singleLine[100];
       	char cfg_param[20];	
	int l;

	char nom_equip[7];
	char MAC_equip[13];
	char server_nom[20]; //TODO: Pot ser més gran?
	int server_port;



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
	
	// Obtain information from the configuration file
	fpointer = fopen(cfg_file,"r");
	char *token;
	while(fgets(singleLine,100,fpointer) != NULL)
	{
		
		printf("Single line: %s\n", singleLine);
		token = strtok(singleLine," ");
		printf("Parameter name: %s\n", token);
		token = strtok(NULL," ");
		printf("Parameter value: %s\n", token);
	}


	fclose(fpointer);
	





	return 0;
}






