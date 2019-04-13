#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h> /* TO DO: es pot utilitzar? */
#include <sys/select.h>
#include <sys/wait.h>

#include <string.h>
#include <signal.h>
#include <getopt.h>

#include <errno.h>


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
	int sock;
	struct sockaddr_in addr_cli, addr_server;
	struct PDU_package tosend_UDP_pack, recv_reg_UDP; /*TODO Utilitzar paquets de dalt a*/
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);
void fill_structures_and_send(struct cfg_data dataconfig, struct hostent *ent, 
							  struct meta_struct *metastruct);
void send_UDP_pack(int debug, struct meta_struct *metastruct);
struct PDU_package recvfrom_UDP(int sock);
void register_req(int debug,struct meta_struct *metastruct);
int register_process(fd_set fdset, struct timeval timeout, int debug,
                    struct meta_struct *metastruct);	
int UDP_answer_treatment(int debug, struct PDU_package recv_reg_UDP);
void debugger(int debug, char message[]);
int select_process(int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct);
void alive(int debug ,struct meta_struct *metastruct);
int authenticate_alive(int debug, struct PDU_package register_pack, struct PDU_package alive_pack);
void console(int debug);
void fork_console(int debug, int fdcons[2], int fdcli[2]);
void check_console_command(int sock);



/* TODO: Implementar debugguer a cada funció en ves de main? */
/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";
	int debug = 0;
	int option;
	int fdcons[2];
	int fdcli[2];

    struct meta_struct metastruct;
	struct cfg_data dataconfig;
	struct hostent *ent;
    ent = malloc(sizeof(ent));

    memset(&metastruct,0,sizeof (struct meta_struct));

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

	debugger(debug, "ESTAT: DISCONNECTED");
	debugger(debug, "Collecting configuration data");
	dataconfig = collect_config_data(cfg_file);
	strcpy(metastruct.tosend_UDP_pack.nom_equip,dataconfig.nom_equip); 

	fork_console(debug, fdcons, fdcli);

	debugger(debug, "Opening UDP socket");
	/* Opens UDP socket */
	if((metastruct.sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}


	while(1){
		register_req(debug, &metastruct);
		fill_structures_and_send(dataconfig, ent, &metastruct);	
		alive(debug, &metastruct);
	}

}

void close_pipes(){
	
}

void check_console_command(int sock)
{
	char word[5];
	fd_set fdset;
	struct timeval timeout;
	
	FD_ZERO(&fdset);
    FD_SET(0, &fdset);
	timeout.tv_usec = 0;
	timeout.tv_sec = 0;
    if(select(8, &fdset, NULL, NULL, &timeout) != 0){
		fgets(word, 5, stdin);
		/*fprintf(stderr,"WORD: %s\n",word);*/
		if(strcmp(word,"quit") == 0){
			wait(NULL);
			close(sock);
			exit(1);
		}
	}
}

void fork_console(int debug, int fdcons[2],int fdcli[2])
{
	pid_t pid;
	if(pipe(fdcons) == -1 || pipe(fdcli) == -1){
		debugger(debug, "Pipe error"); 
		exit(1);
	}
	pid = fork();
	if (pid == 0){
		close(fdcons[0]);
		dup2(fdcons[1], 1);
		close(fdcons[1]);
		console(debug);
	
	}else if(pid != -1){
		close(fdcons[1]);
		dup2(fdcons[0],0);
		close(fdcons[0]);
	}else{
		perror("Error amb el fork del procés de la consola");
		exit(1);
	}
}
void console(int debug)
{
	int quit = 0;
	char word[5];
	while(quit == 0){
		fprintf(stderr,">");
		fgets(word, sizeof(word), stdin);
		
		if (write(1, "pipetest", strlen("pipetest")) == -1){
			if(errno = EPIPE){
				exit(1);
			}
		}
		if(strcmp(word,"quit") == 0){
			quit = 1;
			write(1, word, strlen(word)+1);
			fprintf(stderr, "El client finalitzarà en uns instants\n");
		}else if(strcmp(word,"\n") != 0){
			fprintf(stderr, "Comanda incorrecta\n");
		}
	}
	exit(1);
}

void alive(int debug, struct meta_struct *metastruct)
{
	int i = 0;
	fd_set fdset;
	struct timeval timeout;
	struct PDU_package recv_alive_UDP;
	debugger(debug, "Estat: ALIVE");
	metastruct->tosend_UDP_pack.tipus_paquet = 0x10;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,
					metastruct->recv_reg_UDP.num_aleatori);
	while(i < u){
		check_console_command(metastruct->sock);
		debugger(debug, "Enviat ALIVE_INF");
		send_UDP_pack(debug, metastruct);
		sleep(r);
		
    	FD_ZERO(&fdset);
    	FD_SET(metastruct->sock, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	if(select(8, &fdset, NULL, NULL, &timeout) == 0){
			debugger(debug, "No s'ha rebut resposta a ALIVE_INF");
			i++;
		}else{
			recv_alive_UDP = recvfrom_UDP(metastruct->sock);
			if(UDP_answer_treatment(debug, recv_alive_UDP) == 1 &&
			   authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0){
				i = 0;
			}else if(UDP_answer_treatment(debug, recv_alive_UDP) == 2 &&
					 authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0){
				i = u;				
			}else{
				i++;
			}
		}
	}
}
int authenticate_alive(int debug,struct PDU_package register_pack, struct PDU_package alive_pack)
{
	debugger(debug, "Autenticant el paquet Alive rebut");
	if(strcmp(register_pack.MAC_addr, alive_pack.MAC_addr) != 0 || 
	   strcmp(register_pack.nom_equip, alive_pack.nom_equip) != 0){
		   debugger(debug, "Paquet Alive no correspon al servidor registrat, paquet ignorat ");
		   return 1;
	}else{
		return 0;
	}
}
void register_req(int debug, struct meta_struct *metastruct)
{
	int i;
	int answ = 0;
	struct timeval timeout;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(metastruct->sock, &fdset);
	timeout.tv_usec = 0;
	metastruct->tosend_UDP_pack.tipus_paquet = 0x00;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,"000000");


	debugger(debug, "Començant procés de registre");
	debugger(debug, "ESTAT: WAIT_REG");
	answ = register_process(fdset, timeout, debug, metastruct);
	for(i = 0; i<(q-1) && answ != 1; i++)
	{
		debugger(debug, "PROCÉS DE REGISTRE FET, ESPERANT PEL SEGÜENT");			
		sleep(s);
		
		debugger(debug, "Començant procés de registre");
		answ = register_process(fdset, timeout, debug, metastruct);
	}
	if(answ == 0)
	{
		fprintf(stderr, "Register answer time expired");
		exit(-2);
	}
}
/*TODO: Mirar si hi ha problemes de punters amb paràmetres d'entrada*/
int register_process(fd_set fdset, struct timeval timeout, int debug,
                    struct meta_struct *metastruct)
{
	int i;
	int h = 1;
	int answ = 0;
	timeout.tv_sec = t;
	send_UDP_pack(debug, metastruct);
	debugger(debug, "Enviat REGISTER_REQ");
	for(i = 1; i<p && answ == 0;i++){
		if(i>=n && (i-n+1)<m){
			h++;
		}
		timeout.tv_sec = h*t;
        answ = select_process(debug, fdset, timeout, metastruct);
	}
	return answ;
}
int select_process(int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct)
{
    int answ = 0;
    FD_ZERO(&fdset);
    FD_SET(metastruct->sock, &fdset);
    if(select(8, &fdset, NULL, NULL, &timeout) == 0){
			send_UDP_pack(debug, metastruct);
			debugger(debug, "Enviat REGISTER_REQ");
			check_console_command(metastruct->sock);
	}else{
			debugger(debug, "Rebuda resposta a REGISTER_REQ");
			metastruct->recv_reg_UDP =  recvfrom_UDP(metastruct->sock);
			answ = UDP_answer_treatment(debug, metastruct->recv_reg_UDP);
	}
    return answ;
}
int UDP_answer_treatment(int debug, struct PDU_package recv_reg_UDP) /*TODO: canviar a switch case*/
{
	if(recv_reg_UDP.tipus_paquet == 0x01){
		debugger(debug, "Paquet rebut, REGISTER_ACK");
		debugger(debug, "ESTAT: REGISTERED");
		return 1;
	}else if(recv_reg_UDP.tipus_paquet == 0x02){
		debugger(debug, "Paquet rebut, REGISTER_NACK");
		return 2;
	}else if(recv_reg_UDP.tipus_paquet == 0x03){
		debugger(debug, "Paquet rebut, REGISTER_REJ");
		debugger(debug, "ESTAT: DISCONNECTED");
		fprintf(stderr,"El registre ha estat rebutjat. Motiu: %s\n",recv_reg_UDP.dades);
		exit(-1);
	}else if(recv_reg_UDP.tipus_paquet == 0x09){
		debugger(debug, "Paquet rebut, ERROR");
		debugger(debug, "ESTAT: DISCONNECTED");
		debugger(debug,"Error de protocol");
		exit(-2);
	}else if(recv_reg_UDP.tipus_paquet == 0x11){
		debugger(debug, "Paquet rebut, ALIVE_ACK");
		return 1;
	}else if(recv_reg_UDP.tipus_paquet == 0x12){
		debugger(debug, "Paquet rebut, ALIVE_NACK");
		return 0;
	}else if(recv_reg_UDP.tipus_paquet == 0x13){
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
		fprintf(stderr,"DEBUG ==> %s\n>",message);
	}
}
struct PDU_package recvfrom_UDP(int sock)
{
	struct PDU_package recv_reg_UDP;
	int a = recvfrom(sock, &recv_reg_UDP,sizeof(recv_reg_UDP),0,
                    (struct sockaddr *)0, (int )0);
	if(a<0)
	{
		perror("Error al rebre informacó des del socket UDP");
		exit(-1);
	}
	return recv_reg_UDP;
}
/* Sends the register through the socket sock the tosend_UDP_pack to the addr_server address */
void send_UDP_pack(int debug, struct meta_struct *metastruct)
{
		if(sendto(metastruct->sock, &metastruct->tosend_UDP_pack,
			sizeof(metastruct->tosend_UDP_pack)+1,0, 
			(struct sockaddr*) &metastruct->addr_server, sizeof(metastruct->addr_server)) < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}
}

/* Fills the client address struct for the binding, binds, Gets the IP of the host by its name */
/* TODO: mirar si es pot fer al principi, quan s'inicialitzen les structs */
/* TODO: mirar si fer memset */
void fill_structures_and_send(struct cfg_data dataconfig, struct hostent *ent, 
							  struct meta_struct *metastruct)
{
        metastruct->addr_cli.sin_family = AF_INET;
        metastruct->addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
        metastruct->addr_cli.sin_port = htons(0);
        if(bind(metastruct->sock, (struct sockaddr *)&metastruct->addr_cli,
                       sizeof(struct sockaddr_in))<0)
        {
                perror("Error amb el binding del socket:");
                exit(-1);
        }

        ent = gethostbyname(dataconfig.nom_server);
		
        metastruct->addr_server.sin_family = AF_INET;
        metastruct->addr_server.sin_addr.s_addr = (((struct in_addr *)ent->h_addr)
                                        ->s_addr);
        metastruct->addr_server.sin_port = htons(dataconfig.port_server);

        metastruct->tosend_UDP_pack.tipus_paquet = 0x00;
        strcpy(metastruct->tosend_UDP_pack.nom_equip,dataconfig.nom_equip);
        strcpy(metastruct->tosend_UDP_pack.MAC_addr,dataconfig.MAC_equip);
        strcpy(metastruct->tosend_UDP_pack.num_aleatori,"000000");
        strcpy(metastruct->tosend_UDP_pack.dades,"");
}

/* Function that collects configuration data */
struct cfg_data collect_config_data(char cfg_file[])
{
	/* Obtain information from the configuration file */
	FILE *fpointer;
        char singleLine[100];
        char *cfg_param; /* TO DO: millorar */ 

	struct cfg_data dataconfig;

	if((fpointer = fopen(cfg_file,"r")) == NULL){
		fprintf(stderr, "File does not exist\n");
		exit(1);

	}
        while(fgets(singleLine,100,fpointer) != NULL)
        {
                cfg_param = strtok(singleLine," ");             
                if(strcmp(cfg_param,"Nom") == 0)
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
                        fprintf(stderr, "Error in the content of %s\n",cfg_file );
                        fprintf(stderr, "Terminating process\n");
                        exit(-1);
                }
	}
        fclose(fpointer);
	return dataconfig;
}