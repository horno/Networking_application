#include <stdio.h>
#include <stdlib.h>

#include <string.h>


/* Main function */
int main(int argc, char *argv[])
{
	char cfg_file[] = "client.cfg";

	FILE *fpointer;
	char singleLine[100];
       	char *cfg_param;	

	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20]; //TODO: Pot ser més gran?
	int port_server;



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
	while(fgets(singleLine,100,fpointer) != NULL)
	{
		cfg_param = strtok(singleLine," ");		
		if(strcmp(cfg_param,"nom") == 0)
		{
			cfg_param = strtok(NULL," ");
			strcpy(nom_equip, cfg_param);
			//printf("DEBUG.cfg_param: %s\n", cfg_param);
			//printf("DEBUG.nom_equip: %s\n", nom_equip);
		}
		else if(strcmp(cfg_param,"MAC") == 0)
		{
			cfg_param = strtok(NULL," ");
                        strcpy(MAC_equip,cfg_param);
		}
		else if(strcmp(cfg_param,"Server") == 0)
		{
			cfg_param = strtok(NULL," ");
                        strcpy(nom_server,cfg_param);
		}
		else if(strcmp(cfg_param,"Server_port") == 0)
		{
			cfg_param = strtok(NULL," ");
                        port_server = atoi(cfg_param);
		}	
		else
		{
			printf("Error in the content of %s\n",cfg_file );
			printf("Terminating process\n");
			exit(-1);
		}
	}
        printf("Nom equip: %s\n",nom_equip);
	printf("MAC equip: %s\n",MAC_equip);
        printf("Nom servidor: %s\n",nom_server);
        printf("Port servidor: %d\n",port_server);
	
	fclose(fpointer);
	





	return 0;
}






