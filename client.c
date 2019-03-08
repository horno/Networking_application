#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> /* TO DO: es pot utilitzar? */

#include <string.h>


#define h_addr h_addr_list[0] /* Backward compatibility */
 
/* Structures */
struct cfg_data{ /*TO DO: rename? */
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20]; /*TODO Pot ser més gran? */
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
void fill_structures_and_send(int sock, struct sockaddr_in *addr_cli,
					struct hostent *ent, struct cfg_data dataconfig,
					struct sockaddr_in *addr_server,
					struct register_package *register_pack);

/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";

	struct cfg_data dataconfig;
	struct sockaddr_in	addr_cli,addr_server;
	struct hostent *ent = malloc(sizeof(*ent));

	struct register_package register_pack;

	int sock;

	if(argc > 1)
	{
		if(strcmp(argv[1],"-c") == 0)
		{
			cfg_file = change_cfg_filename(argc, argv);
		}
	}
	
	/* Function that collects configuration data */

	dataconfig = collect_config_data(cfg_file);
	
	strcpy(register_pack.nom_equip,dataconfig.nom_equip); 

	/* Opens UDP socket */
	if((sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket:");
		exit(-1);
	}

	fill_structures_and_send(sock, &addr_cli, ent, dataconfig, &addr_server, &register_pack);



	/*a = sendto(sock,&register_pack,sizeof(register_pack)+1,0, 
	    (struct sockaddr*) &addr_server, sizeof(addr_server));
		if(a < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}*/




	
	close(sock);
	return 0;
}

/* Fills the client address struct for the binding, binds, Gets the IP of the host by its name */
/* TODO: mirar si es pot fer al principi, quan s'inicialitzen les structs */
/* TODO: mirar si fer memset */
void fill_structures_and_send(int sock, struct sockaddr_in *addr_cli,
					struct hostent *ent, struct cfg_data dataconfig,
					struct sockaddr_in *addr_server,
					struct register_package *register_pack)
{
		int a;

        addr_cli->sin_family = AF_INET;
        addr_cli->sin_addr.s_addr = htonl(INADDR_ANY);
        addr_cli->sin_port = htons(0);
        if(bind(sock, (struct sockaddr *)addr_cli,
                       sizeof(struct sockaddr_in))<0)
        {
                perror("Error amb el binding del socket:");
                exit(-1);
        }

        ent = gethostbyname(dataconfig.nom_server);
		
        addr_server->sin_family = AF_INET;
        addr_server->sin_addr.s_addr = (((struct in_addr *)ent->h_addr)
                                        ->s_addr);
        addr_server->sin_port = htons(dataconfig.port_server);

        register_pack->tipus_paquet = 0x00;
        strcpy(register_pack->nom_equip,dataconfig.nom_equip);
        strcpy(register_pack->MAC_addr,dataconfig.MAC_equip);
        strcpy(register_pack->num_aleatori,"");
        strcpy(register_pack->dades,"");

		a = sendto(sock, register_pack,sizeof(*register_pack)+1,0, 
	    (struct sockaddr*) addr_server, sizeof(*addr_server));
		if(a < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}
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

/* Function that collects configuration data */
struct cfg_data collect_config_data(char cfg_file[])
{
	/* Obtain information from the configuration file */
	FILE *fpointer;
        char singleLine[100];
        char *cfg_param; /* TO DO: millorar */ 

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
