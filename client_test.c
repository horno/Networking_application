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
#define r 3
#define u 3

/* Structures */
struct cfg_data{ /*TO DO: rename? */
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20]; /*TODO Pot ser més gran? */
	int port_server;
};
struct PDU_package{
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
	struct PDU_package tosend_UDP_pack, torecv_UDP_pack; /*TODO Puc usar 1 sol?*/
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);
void fill_structures_and_send(int sock, struct meta_struct *metastruct);
void send_UDP_pack(int sock, struct meta_struct *metastruct);
void recvfrom_UDP(int sock, struct meta_struct *metastruct);
void register_req(int sock, int debug,struct meta_struct *metastruct);
int register_process(fd_set fdset, struct timeval timeout, int sock, int debug,
                    struct meta_struct *metastruct);	
int UDP_answer_treatment(int debug, struct meta_struct metastruct);
void debugger(int debug, char message[]);
int select_process(int sock, int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct);
void alive(int sock, int debug ,struct meta_struct *metastruct);


/* TODO: Implementar debugguer a cada funció en ves de main? */
/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";
	int sock;
	int debug = 0;

    struct meta_struct metastruct;
    metastruct.ent = malloc(sizeof(metastruct.ent));

    memset(&metastruct,0,sizeof (struct meta_struct));

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
	strcpy(metastruct.tosend_UDP_pack.nom_equip,metastruct.dataconfig.nom_equip); 

	debugger(debug, "Opening UDP socket");
	/* Opens UDP socket */
	if((sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}

	fill_structures_and_send(sock, &metastruct);

	while(1){
		register_req(sock, debug, &metastruct);
		alive(sock, debug, &metastruct);
	}

	close(sock);
	return 0;
}

void alive(int sock, int debug, struct meta_struct *metastruct)
{
	int i = 0;
	fd_set fdset;
	struct timeval timeout;
	debugger(debug, "Estat: WAIT_REG");
	metastruct->tosend_UDP_pack.tipus_paquet = 0x10;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,
					metastruct->torecv_UDP_pack.num_aleatori);
	while(i < u){
		debugger(debug, "Enviat ALIVE_INF");
		send_UDP_pack(sock,metastruct);
		sleep(r);
		
    	FD_ZERO(&fdset);
    	FD_SET(sock, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	if(select(8, &fdset, NULL, NULL, &timeout) == 0){
			debugger(debug, "No s'ha rebut resposta a ALIVE_INF");
			i++;
		}else{
			recvfrom_UDP(sock, metastruct);
			if(UDP_answer_treatment(debug, *metastruct) == 1){
				i = 0;
			}else if(UDP_answer_treatment(debug, *metastruct) == 0){
				i++;
			}else{
				i = u;
			}
		}
	}
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
	metastruct->tosend_UDP_pack.tipus_paquet = 0x00;

	debugger(debug, "Començant procés de registre");
	debugger(debug, "ESTAT: WAIT_REG");
	answ = register_process(fdset, timeout, sock, debug, metastruct);
	for(i = 0; i<(q-1) && answ != 1; i++)
	{
		debugger(debug, "PROCÉS DE REGISTRE FET, ESPERANT PEL SEGÜENT");			
		sleep(s);
		
		debugger(debug, "Començant procés de registre");
		answ = register_process(fdset, timeout, sock, debug, metastruct);
	}
	if(answ == 0)
	{
		printf("Register answer time expired");
		exit(-2);
	}
}
/*TODO: Mirar si hi ha problemes de punters amb paràmetres d'entrada*/
int register_process(fd_set fdset, struct timeval timeout, int sock, int debug,
                    struct meta_struct *metastruct)
{
	int i;
	int h = 1;
	int answ = 0;
	timeout.tv_sec = t;
	send_UDP_pack(sock, metastruct);
	debugger(debug, "Enviat REGISTER_REQ");
	for(i = 1; i<p && answ == 0;i++){
		if(i>=n && (i-n+1)<m){
			h++;
		}
		timeout.tv_sec = h*t;
        answ = select_process(sock, debug, fdset, timeout, metastruct);
	}
	return answ;
}
int select_process(int sock, int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct)
{
    int answ = 0;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    if(select(8, &fdset, NULL, NULL, &timeout) == 0){
			send_UDP_pack(sock, metastruct);
			debugger(debug, "Enviat REGISTER_REQ");
	}else{
			recvfrom_UDP(sock, metastruct);
			debugger(debug, "Rebuda resposta a REGISTER_REQ");		
			answ = UDP_answer_treatment(debug, *metastruct);
	}
    return answ;
}

int UDP_answer_treatment(int debug, struct meta_struct metastruct) /*TODO: canviar a switch case*/
{
	if(metastruct.torecv_UDP_pack.tipus_paquet == 0x01){
		debugger(debug, "Paquet rebut, REGISTER_ACK");
		debugger(debug, "ESTAT: REGISTERED");
		return 1;
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x02){
		debugger(debug, "Paquet rebut, REGISTER_NACK");
		return 2;
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x03){
		debugger(debug, "Paquet rebut, REGISTER_REJ");
		debugger(debug, "ESTAT: DISCONNECTED");
		printf("El registre ha estat rebutjat. Motiu: %s\n",metastruct.torecv_UDP_pack.dades);
		exit(-1);
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x09){
		debugger(debug, "Paquet rebut, ERROR");
		debugger(debug, "ESTAT: DISCONNECTED");
		printf("Error de protocol");
		exit(-2);
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x11){
		debugger(debug, "Paquet rebut, ALIVE_ACK");
		debugger(debug, "ESTAT: ALIVE");
		return 1;
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x12){
		debugger(debug, "Paquet rebut, ALIVE_NACK");
		return 0;
	}else if(metastruct.torecv_UDP_pack.tipus_paquet == 0x13){
		debugger(debug, "Paquet rebut, ALIVE_REJ");
		return 2;
	}else{
		debugger(debug,"Paquet rebut, NO IDENTIFICAT");
		exit(-2);
	}
}
void debugger(int debug, char message[])
{
	if(debug == 1){
		printf("DEBUG ==> %s\n",message);
	}
}

void recvfrom_UDP(int sock, struct meta_struct *metastruct)
{
	int a = recvfrom(sock, &metastruct->torecv_UDP_pack,sizeof(metastruct->torecv_UDP_pack),0,
                    (struct sockaddr *)0, (int )0);
	if(a<0)
	{
		perror("Error al rebre informacó des del socket UDP");
		exit(-1);
	}
}

/* Sends the register through the socket sock the tosend_UDP_pack to the addr_server address */
void send_UDP_pack(int sock, struct meta_struct *metastruct)
{
		if(sendto(sock, &metastruct->tosend_UDP_pack,sizeof(metastruct->tosend_UDP_pack)+1,0, 
	    (struct sockaddr*) &metastruct->addr_server, sizeof(metastruct->addr_server)) < 0)
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

        metastruct->tosend_UDP_pack.tipus_paquet = 0x00;
        strcpy(metastruct->tosend_UDP_pack.nom_equip,metastruct->dataconfig.nom_equip);
        strcpy(metastruct->tosend_UDP_pack.MAC_addr,metastruct->dataconfig.MAC_equip);
        strcpy(metastruct->tosend_UDP_pack.num_aleatori,"000000");
        strcpy(metastruct->tosend_UDP_pack.dades,"");
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