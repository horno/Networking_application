#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> //TO DO: es pot utilitzar?

#include <string.h>


/* Structures */
struct cfg_data{ //TO DO: rename?
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20];//TODO Pot ser més gran?
	int port_server;
};
struct register_package{
	unsigned char tipus_paquet;
	char nom_equip[7];
	char MAC_addr[13];
	char num_aleatori[7];
	char dades[50];
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);


/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";

	struct cfg_data dataconfig;
	struct sockaddr_in	addr_cli,addr_server;
	struct hostent *ent;

	int sock;

	int a;

	/* TEMPORAL - DEBUG*/
	char dades[6] = "HEY YA";
	/* TEMPORAL - DEBUG */

	/* argument functionalities */
	if(argc > 1)
	{
		if(strcmp(argv[1],"-c") == 0)
		{
			cfg_file = change_cfg_filename(argc, argv);
		}
	}
	
	/* Calls function that collects configuration data */

	dataconfig = collect_config_data(cfg_file);
	
	/* Opens UDP socket */

	if((sock = socket(AF_INET,SOCK_DGRAM,0))<9)
	{
		perror("Error al obrir el socket:");
		exit(-1);
	}
		
	/* Fills the client address struct for the binding */
	//TO DO: mirar si fer memset
	addr_cli.sin_family = AF_INET;
	addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
	addr_cli.sin_port = htons(0);

	/* Binding */
	if(bind(sock, (struct sockaddr *)&addr_cli,
	               sizeof(struct sockaddr_in))<0)
	{
		perror("Error amb el binding del socket:");
	}

	/* Gets the IP of the host by its name */
	ent = gethostbyname(dataconfig.nom_server);

	/* Fills the server address struct where we send the data */
	//TO DO: mirar si fer memset
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = (((struct in_addr *)ent->h_addr)
					->s_addr);
	addr_server.sin_port = htons(dataconfig.port_server);
	
	/* Sends package to the server */
	a = sendto(sock,dades,strlen(dades)+1,0, //TO DO: +1?
	       	(struct sockaddr*) &addr_server, sizeof(addr_server));
	if(a < 0)
	{
		perror("Error al enviar el paquet");
	}




	
	close(sock);
	return 0;
}
char* change_cfg_filename(int argc, char *argv[])
{
	if(argc != 3)
	{
		printf("Use:\n");
	       	printf("\t%s -c <nom_arxiu>\n",argv[0]);
		exit(-1);
	}
	else
	{
		 return argv[2];
	}
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
