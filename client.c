#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> /* TO DO: es pot utilitzar? */
#include <sys/select.h>

#include <string.h>


#define h_addr h_addr_list[0] /* Backward compatibility */

/* Timeout defines */
#define n 3
#define t 2
#define m 4
#define p 8
#define s 5
#define q 3

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
void send_register_req(int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server);
void recvfrom_register_req(int sock, struct register_package recv_register_pack);
void register_req(int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server, struct register_package recv_register_pack);
int register_process(fd_set fdset, struct timeval timeout, int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server,  struct register_package recv_register_pack);	
void register_answer_treatment();

/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";

	struct cfg_data dataconfig;
	struct sockaddr_in	addr_cli,addr_server;
	struct hostent *ent = malloc(sizeof(*ent));

	struct register_package register_pack, *recv_register_pack = malloc(sizeof(*recv_register_pack));

	int sock;

	/*int a;*/

	/*struct timeval timeout;
	fd_set fdset;*/

	if(argc > 1)
	{
		if(strcmp(argv[1],"-c") == 0)
		{
			cfg_file = change_cfg_filename(argc, argv);
		}
	}

	dataconfig = collect_config_data(cfg_file);
	strcpy(register_pack.nom_equip,dataconfig.nom_equip); 

	/* Opens UDP socket */
	if((sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}

	fill_structures_and_send(sock, &addr_cli, ent, dataconfig, &addr_server, &register_pack);
	send_register_req(sock, &register_pack, &addr_server);
	
	/*FD_ZERO(&fdset);
	FD_SET(sock, &fdset);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;*/

	/*a = select(8, &fdset, NULL, NULL, &timeout);
	if(a == 0)
	{
		printf("Temps de resposta expirat\n");
	}else
	{
		printf("Select efectuat correctament\n");
		recvfrom_register_req(sock, *recv_register_pack);
	}*/


	close(sock);
	return 0;
}
void register_req(int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server, struct register_package recv_register_pack)
{
	int i;
	int answ;
	struct timeval timeout;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);

	timeout.tv_usec = 0;

	answ  = register_process(fdset, timeout, sock, register_pack, addr_server, recv_register_pack);
	for(i = 0; i<(q-1) && answ == 0; i++)
	{
		timeout.tv_sec = s;

		if (select(8, &fdset, NULL, NULL, &timeout) == 0) 
		{
			answ = register_process(fdset, timeout, sock, register_pack, addr_server, recv_register_pack);
		}
		else
		{
			recvfrom_register_req(sock, recv_register_pack);
			answ = 1;
		}
	}
	if(answ != 0)
	{
			register_answer_treatment();
	}
	else
	{
		printf("Register answer time expired");
		exit(-2);
	}
}
/*TODO: Mirar si hi ha problemes de punters amb paràmetres d'entrada*/
int register_process(fd_set fdset, struct timeval timeout, int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server,  struct register_package recv_register_pack)
{
	int h = 1;
	int i;
	int a;
	int answ = 0;
	timeout.tv_sec = t;
	for(i = 0; i<p && answ == 0;i++){
		timeout.tv_sec = h*t;
		a = select(8, &fdset, NULL, NULL, &timeout);
		if(a == 0){
			send_register_req(sock, register_pack, addr_server);
		}else{
			recvfrom_register_req(sock, recv_register_pack);
			answ = 1;
		}
		if(i>n && (i-n)<m){
			h++;
		}
	}
	return answ;
}

void register_answer_treatment()
{
	printf("Yay!...treatment!");
}

void recvfrom_register_req(int sock, struct register_package recv_register_pack)
{
	int a = recvfrom(sock,&recv_register_pack,sizeof(recv_register_pack),0,(struct sockaddr *)0, (int )0);
	if(a<0)
	{
		perror("Error al rebre informacó des del socket UDP");
		exit(-1);
	}

	printf("Dades: %s\n", recv_register_pack.dades);
}


/* Sends the register through the socket sock the register_pack to the addr_server address */
void send_register_req(int sock, struct register_package *register_pack,
						struct sockaddr_in *addr_server)
{
	int a;
	a = sendto(sock, register_pack,sizeof(*register_pack)+1,0, 
	    (struct sockaddr*) addr_server, sizeof(*addr_server));
		if(a < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}

}

/* Fills the client address struct for the binding, binds, Gets the IP of the host by its name */
/* TODO: mirar si es pot fer al principi, quan s'inicialitzen les structs */
/* TODO: mirar si fer memset */
void fill_structures_and_send(int sock, struct sockaddr_in *addr_cli,
					struct hostent *ent, struct cfg_data dataconfig,
					struct sockaddr_in *addr_server,
					struct register_package *register_pack)
{
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
        strcpy(register_pack->num_aleatori,"000000");
        strcpy(register_pack->dades,"");
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
