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
struct meta_struct{
	struct sockaddr_in addr_cli, addr_server;
	struct cfg_data dataconfig;
	struct hostent *ent;
	struct register_package register_pack, *recv_register_pack;
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);
void fill_structures_and_send(int sock, struct meta_struct *metastruct);
void send_register_req(int sock, struct meta_struct *metastruct);
void recvfrom_register_req(int sock, struct meta_struct metastruct);
void register_req(int sock, int debug,struct meta_struct *metastruct);
int register_process(fd_set fdset, struct timeval timeout, int sock, int debug,
                    struct meta_struct *metastruct);	
void register_answer_treatment();
void debugger(int debug, char message[]);


/* TODO: Implementar debugguer a cada funció en ves de main? */
/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";
	int sock;
	int debug = 0;

    struct meta_struct metastruct;
    metastruct.ent = malloc(sizeof(metastruct.ent)); /*revisar*/
    metastruct.recv_register_pack = malloc(sizeof(metastruct.recv_register_pack));

	if(argc > 1)
	{
		if(strcmp(argv[1],"-c") == 0)
		{
			cfg_file = change_cfg_filename(argc, argv);
		}
		else if(strcmp(argv[1],"-d") == 0)
		{
			debug = 1;
		}
	}
	debugger(debug, "Collecting configuration data");
	metastruct.dataconfig = collect_config_data(cfg_file);
	strcpy(metastruct.register_pack.nom_equip,metastruct.dataconfig.nom_equip); 

	debugger(debug, "Opening socket");
	/* Opens UDP socket */
	if((sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}

	fill_structures_and_send(sock, &metastruct);

	register_req(sock, debug, &metastruct);
	/*send_register_req(sock, &register_pack, &addr_server);*/
	
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
void register_req(int sock, int debug, struct meta_struct *metastruct)
{
	int i;
	int answ = 0;
	struct timeval timeout;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);

	timeout.tv_usec = 0;

	debugger(debug, "Començant procés de registre");
	answ  = register_process(fdset, timeout, sock, debug, metastruct);
	for(i = 0; i<(q-1) && answ == 0; i++)
	{
		timeout.tv_sec = s;
		debugger(debug, "PRIMER PROCÉS DE REGISTRE FET, ESPERANT PEL SEGÜENT");
		if (select(8, &fdset, NULL, NULL, &timeout) == 0) 
		{
			debugger(debug, "Començant procés de registre");
			answ = register_process(fdset, timeout, sock, debug, metastruct);
		}
		else
		{
			recvfrom_register_req(sock, *metastruct);
			answ = 1;
			debugger(debug, "Rebuda resposta a REGISTER_REQ");

		}
	}
	if(answ != 0)
	{
			debugger(debug, "Començant tractament de resposta de REGISTER_REQ");
			register_answer_treatment();
	}
	else
	{
		printf("Register answer time expired");
		exit(-2);
	}
}
/*TODO: Mirar si hi ha problemes de punters amb paràmetres d'entrada*/
int register_process(fd_set fdset, struct timeval timeout, int sock, int debug,
                    struct meta_struct *metastruct)
{
	int h = 1;
	int i;
	int a;
	int answ = 0;
	timeout.tv_sec = t;
	send_register_req(sock, metastruct);
	debugger(debug, "Enviat REGISTER_REQ ");
	for(i = 1; i<p && answ == 0;i++){
		if(i>=n && (i-n+1)<m){
			h++;
		}
		timeout.tv_sec = h*t;
		a = select(8, &fdset, NULL, NULL, &timeout);
		if(a == 0){
			send_register_req(sock, metastruct);
			debugger(debug, "Enviat REGISTER_REQ ");
		}else{
			recvfrom_register_req(sock, *metastruct);
			answ = 1;
			debugger(debug, "Rebuda resposta a REGISTER_REQ");
		}
		
	}
	return answ;
}

void register_answer_treatment()
{
	printf("Yay!...treatment!\n");
}
void debugger(int debug, char message[])
{
	if(debug == 1){
		printf("DEBUG ==> %s\n",message);
	}
}

void recvfrom_register_req(int sock, struct meta_struct metastruct)
{
	int a = recvfrom(sock,&metastruct.recv_register_pack,sizeof(metastruct.recv_register_pack),0,
                    (struct sockaddr *)0, (int )0);
	if(a<0)
	{
		perror("Error al rebre informacó des del socket UDP");
		exit(-1);
	}

	printf("Dades: %s\n", metastruct.recv_register_pack->dades);
}


/* Sends the register through the socket sock the register_pack to the addr_server address */
void send_register_req(int sock, struct meta_struct *metastruct)
{
	int a;
	a = sendto(sock, &metastruct->register_pack,sizeof(metastruct->register_pack)+1,0, 
	    (struct sockaddr*) &metastruct->addr_server, sizeof(metastruct->addr_server));
		if(a < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}

}

/* Fills the client address struct for the binding, binds, Gets the IP of the host by its name */
/* TODO: mirar si es pot fer al principi, quan s'inicialitzen les structs */
/* TODO: mirar si fer memset */
void fill_structures_and_send(int sock, struct meta_struct *metastruct)
{
        metastruct->addr_cli.sin_family = AF_INET;
        metastruct->addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
        metastruct->addr_cli.sin_port = htons(0);
        if(bind(sock, (struct sockaddr *)&metastruct->addr_cli,
                       sizeof(struct sockaddr_in))<0)
        {
                perror("Error amb el binding del socket:");
                exit(-1);
        }

        metastruct->ent = gethostbyname(metastruct->dataconfig.nom_server);
		
        metastruct->addr_server.sin_family = AF_INET;
        metastruct->addr_server.sin_addr.s_addr = (((struct in_addr *)metastruct->ent->h_addr)
                                        ->s_addr);
        metastruct->addr_server.sin_port = htons(metastruct->dataconfig.port_server);

        metastruct->register_pack.tipus_paquet = 0x00;
        strcpy(metastruct->register_pack.nom_equip,metastruct->dataconfig.nom_equip);
        strcpy(metastruct->register_pack.MAC_addr,metastruct->dataconfig.MAC_equip);
        strcpy(metastruct->register_pack.num_aleatori,"000000");
        strcpy(metastruct->register_pack.dades,"");
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
