#include <stdio.h>
#include <stdlib.h>

#include <string.h>


/* Structures */
struct cfg_data{
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20];//TODO Pot ser més gran?
	int port_server;
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);


/* Main function */
int main(int argc, char *argv[])
{
	char cfg_file[] = "client.cfg";

	struct cfg_data dataconfig;

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
	
	dataconfig = collect_config_data(cfg_file);
	printf("%s\n",dataconfig.nom_equip);
	printf("%s\n",dataconfig.MAC_equip);
	printf("%s\n",dataconfig.nom_server);
	printf("%d\n",dataconfig.port_server);

	return 0;
}


struct cfg_data collect_config_data(char cfg_file[])
{
	// Obtain information from the configuration file
	FILE *fpointer;
        char singleLine[100];
        char *cfg_param; //TODO: millorar  

	struct cfg_data dataconfig;

	fpointer = fopen(cfg_file,"r");
        while(fgets(singleLine,100,fpointer) != NULL)
        {
                cfg_param = strtok(singleLine," ");             
                if(strcmp(cfg_param,"nom") == 0)
                {
                        strcpy(dataconfig.nom_equip, strtok(NULL,"\n"));
	       	}
                else if(strcmp(cfg_param,"MAC") == 0)
                {
                        strcpy(dataconfig.MAC_equip,strtok(NULL,"\n"));
                }
                else if(strcmp(cfg_param,"Server") == 0)
                {
                        strcpy(dataconfig.nom_server,strtok(NULL,"\n"));
                }
                else if(strcmp(cfg_param,"Server-port") == 0)
                {
                        dataconfig.port_server = atoi(strtok(NULL,"\n"));
                }       
                else
                {
                        printf("Error in the content of %s\n",cfg_file );
                        printf("Terminating process\n");
                        exit(-1);
                }
	}
        fclose(fpointer);
	return dataconfig;
}
