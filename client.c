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
#define w 4

/* Structures */
struct cfg_data{ /*TO DO: rename? */
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[20]; /*TODO Pot ser més gran? */
	int port_server;
};
struct PDU_UDP_package{
	unsigned char tipus_paquet;
	char nom_equip[7];
	char MAC_addr[13];
	char num_aleatori[7];
	char dades[50];
};
struct PDU_TCP_package{
	unsigned char tipus_paquet;
	char nom_equip[7];
	char MAC_addr[13];
	char num_aleatori[7];
	char dades[150];
};
struct meta_struct{
	int sock;
	struct sockaddr_in addr_cli, addr_server;
	struct PDU_UDP_package tosend_UDP_pack, recv_reg_UDP; /*TODO Utilitzar paquets de dalt a*/
};
struct console_needs{
	int fdcons[2];
	int fdcli[2];
	int tcp_port;
	char boot_file[20];
	struct PDU_UDP_package reg_info;
	in_addr_t  s_addr;
};


/* Function declarations */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);
void fill_structures_and_send(struct cfg_data dataconfig, struct hostent *ent, 
							  struct meta_struct *metastruct);
void send_UDP_pack(int debug, struct meta_struct *metastruct);
struct PDU_UDP_package recvfrom_UDP(int sock);
int register_req(int debug,struct meta_struct *metastruct);
int register_process(fd_set fdset, struct timeval timeout, int debug,
                    struct meta_struct *metastruct);	
int UDP_answer_treatment(int debug, struct PDU_UDP_package recv_reg_UDP);
void debugger(int debug, char message[]);
int select_process(int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct);
void alive(int debug ,struct meta_struct *metastruct, struct console_needs *cons_struct);
int authenticate_alive(int debug, struct PDU_UDP_package register_pack, struct PDU_UDP_package alive_pack);
void console(int debug, struct console_needs cons_struct);
void fork_console(int debug, struct console_needs *cons_struct);
void check_console_command(int sock, int fdcons[2]);
void close_commandline(int fdcli[2]);
void sendconf(int debug, struct console_needs cons_struct);
void pass_info(struct console_needs *cons_struct, struct meta_struct metastruct);



/* TODO: Implementar debugguer a cada funció en ves de main? */
/* Main function */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";
	char boot_file[] = "boot.cfg";
	int debug = 0;
	int option;

    struct meta_struct metastruct;
	struct cfg_data dataconfig;
	struct hostent *ent;
	struct console_needs cons_struct;
    ent = malloc(sizeof(ent));

    memset(&metastruct,0,sizeof (struct meta_struct));

	while((option = getopt(argc, argv, "dc:f:")) != -1){
		switch (option)
		{
			case 'c':
				cfg_file = optarg;
				break;
			case 'f':
				strcpy(boot_file, optarg);
				break;
			case 'd':
				debug = 1;
				break;
			default:
				fprintf(stderr, "Argument incorrecte");
				exit(1);
		}

	}
	boot_file[sizeof(boot_file)] = '\0';
	debugger(debug, "ESTAT: DISCONNECTED");
	debugger(debug, "Collecting configuration data");
	dataconfig = collect_config_data(cfg_file);
	strcpy(metastruct.tosend_UDP_pack.nom_equip,dataconfig.nom_equip); 
	strcpy(cons_struct.boot_file, boot_file);


	debugger(debug, "Opening UDP socket");
	/* Opens UDP socket */
	if((metastruct.sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}

	fill_structures_and_send(dataconfig, ent, &metastruct);
	
	while(1){
		cons_struct.tcp_port = register_req(debug, &metastruct);
		alive(debug, &metastruct, &cons_struct);
	}

}
void close_commandline(int fdcli[2]){
	write(fdcli[1],"close",sizeof("close"));
}

void check_console_command(int sock, int fdcons[2])
{
	char buff[20];
	fd_set fdset;
	struct timeval timeout;
	FD_ZERO(&fdset);
	FD_SET(fdcons[0], &fdset);
	timeout.tv_usec = 0;
	timeout.tv_sec = 0;
	select(fdcons[0]+1, &fdset, NULL, NULL, &timeout);
	if(FD_ISSET(fdcons[0], &fdset)){
		read(fdcons[0], buff, sizeof(buff));
		if(strcmp(buff,"quit")){
			close(sock);
			exit(1);
		}	
	}
}

void fork_console(int debug, struct console_needs *cons_struct)
{
	pid_t pid;
	if(pipe(cons_struct->fdcons) == -1 || pipe(cons_struct->fdcli) == -1){
		fprintf(stderr,"Pipe error"); 
		exit(1);
	}
	pid = fork();	
	if (pid == 0){
		close(cons_struct->fdcons[0]);
		close(cons_struct->fdcli[1]);
		console(debug, *cons_struct);
	
	}else if(pid != -1){	
		close(cons_struct->fdcons[1]);
		close(cons_struct->fdcli[0]);
	}else{
		fprintf(stderr,"Error amb el fork del procés de la consola");
		exit(1);
	}
}
void console(int debug, struct console_needs cons_struct)
{
	char buff[20];
	struct timeval timeout;
	fd_set fdset;

	fprintf(stderr,"consola oberta!\n");
	while(1){
    	FD_ZERO(&fdset);
    	FD_SET(cons_struct.fdcli[0], &fdset);
		FD_SET(0, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	select(cons_struct.fdcli[0]+1, &fdset, NULL, NULL, &timeout);
		if(FD_ISSET(cons_struct.fdcli[0],&fdset)){
			read(cons_struct.fdcli[0], buff, sizeof(buff));
			if(strcmp(buff,"close\n") || strcmp(buff,"close")){
				fprintf(stderr,"Tancant consola\n");
				exit(1);
			}
		}
		if(FD_ISSET(0,&fdset)){
			fgets(buff, sizeof(buff), stdin);
			if(strcmp(buff,"quit\n")==0 || strcmp(buff,"quit") == 0){
				write(cons_struct.fdcons[1],buff,4);
				fprintf(stderr, "Tancant consola\n");
				fprintf(stderr, "El client es tancarà en uns instants\n");
				exit(1);
			}else if(strcmp(buff, "send-conf\n")== 0 || strcmp(buff, "send-conf") == 0){
				fprintf(stderr, "I have to send conf!\n");
				sendconf(debug, cons_struct);
			}else if(strcmp(buff, "get-conf\n") == 0 || strcmp(buff, "get-conf") == 0){
				debugger(debug,"Get-conf not implemented yet!\n");
			}else if(strcmp(buff, "\n") != 0){
				fprintf(stderr, "Comanda incorrecta\n");
			}
			fprintf(stderr, ">");
		}
	}
}

void sendconf(int debug, struct console_needs cons_struct){
	FILE *fp;
	int sz;
	int sockfd;
	struct PDU_TCP_package send_tcp_pack;
	struct PDU_TCP_package recv_tcp_pack;
	struct sockaddr_in addr_server;
	struct timeval timeout;
	fd_set fdset;
	if((fp = fopen(cons_struct.boot_file, "r")) == NULL){
		fprintf(stderr, "Problem with the file");
		exit(-1);
	}
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	rewind(fp);
	sprintf(send_tcp_pack.dades, "%s,%d", cons_struct.boot_file, sz);

	send_tcp_pack.tipus_paquet = 0x20;
	strcpy(send_tcp_pack.MAC_addr,cons_struct.reg_info.MAC_addr);
	strcpy(send_tcp_pack.nom_equip,cons_struct.reg_info.nom_equip);
	strcpy(send_tcp_pack.num_aleatori,cons_struct.reg_info.num_aleatori);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error al obrir el socket");
		exit(-1);
	} 
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = cons_struct.s_addr;
	addr_server.sin_port = htons(cons_struct.tcp_port);

	if (connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) != 0) { 
    	fprintf(stderr,"La connexió amb el servidor ha fallat\n"); 
    	exit(0); 
    } 
	debugger(debug, "Connectant socket TCP");
	send(sockfd, &send_tcp_pack, sizeof(send_tcp_pack)+1,0);
	debugger(debug, "Enviat SEND_FILE");

	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	timeout.tv_sec = w;
	timeout.tv_usec = 0;
	if(select(sockfd+1, &fdset, NULL, NULL, &timeout) == 0){
		debugger(debug, "Temps d'espera de SEND_FILE vençut");		
	}else{
		if(recv(sockfd, &recv_tcp_pack, sizeof(recv_tcp_pack)+1, 0) < 0){
			fprintf(stderr, "Error al enviar paquet TCP\n");
			exit(-1);
		}
		if(recv_tcp_pack.tipus_paquet == 0x21){
			debugger(debug, "Rebut SEND_ACK");
			fprintf(stderr, "YAY! Send accepted");
		}else{
			fprintf(stderr, "Send not accepted :(");
			debugger(debug, "Rebut paquet que no és SEND_ACK");
		}
	}



	close(sockfd);


	/*int answ = 0;
    FD_ZERO(&fdset);
    FD_SET(metastruct->sock, &fdset);
    if(select(metastruct->sock+1, &fdset, NULL, NULL, &timeout) == 0){
			send_UDP_pack(debug, metastruct);
			debugger(debug, "Enviat REGISTER_REQ");
	}else{
			debugger(debug, "Rebuda resposta a REGISTER_REQ");
			metastruct->recv_reg_UDP =  recvfrom_UDP(metastruct->sock);
			answ = UDP_answer_treatment(debug, metastruct->recv_reg_UDP);
	}
    return answ;


	if(sendto(metastruct->sock, &metastruct->tosend_UDP_pack,
			sizeof(metastruct->tosend_UDP_pack)+1,0, 
			(struct sockaddr*) &metastruct->addr_server, sizeof(metastruct->addr_server)) < 0)
		{
			perror("Error al enviar el paquet");
			exit(-1);
		}*/



	/*char singleLine[100];*/
	/*if((fpointer = fopen(cfg_file,"r")) == NULL){
	while(fgets(singleLine,100,fpointer) != NULL)*/

}

void alive(int debug, struct meta_struct *metastruct, struct console_needs *cons_struct)
{
	int i = 0;
	int first = 1;
	fd_set fdset;
	struct timeval timeout;
	struct PDU_UDP_package recv_alive_UDP;
	metastruct->tosend_UDP_pack.tipus_paquet = 0x10;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,
					metastruct->recv_reg_UDP.num_aleatori);
	while(i < u){
		debugger(debug, "Enviat ALIVE_INF");
		send_UDP_pack(debug, metastruct);
		sleep(r);
		if(!first){
			check_console_command(metastruct->sock, cons_struct->fdcons);
		}
		FD_ZERO(&fdset);
    	FD_SET(metastruct->sock, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	if(select(metastruct->sock+1, &fdset, NULL, NULL, &timeout) == 0){
			debugger(debug, "No s'ha rebut resposta a ALIVE_INF");
			i++;
		}else{
			recv_alive_UDP = recvfrom_UDP(metastruct->sock);
			if(UDP_answer_treatment(debug, recv_alive_UDP) == 1 &&
			   authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0){
				i = 0;
				if(first == 1){
					pass_info(cons_struct,*metastruct);
					fork_console(debug, cons_struct);
					debugger(debug, "Rebut primer alive!");
					debugger(debug, "ESTAT: ALIVE");
				}
				first = 0;
			}else if(UDP_answer_treatment(debug, recv_alive_UDP) == 2 &&
					 authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0){
				i = u;				
			}else{
				i++;
			}
		}
	}
	if(!first){
		close_commandline(cons_struct->fdcli);
	}
}
void pass_info(struct console_needs *cons_struct, struct meta_struct metastruct){
	strcpy(cons_struct->reg_info.MAC_addr,metastruct.tosend_UDP_pack.MAC_addr);
	strcpy(cons_struct->reg_info.nom_equip,metastruct.tosend_UDP_pack.nom_equip);
	strcpy(cons_struct->reg_info.num_aleatori,metastruct.tosend_UDP_pack.num_aleatori);
	
	cons_struct->s_addr = metastruct.addr_server.sin_addr.s_addr;
}
int authenticate_alive(int debug,struct PDU_UDP_package register_pack, struct PDU_UDP_package alive_pack)
{
	debugger(debug, "Autenticant el paquet Alive rebut");
	if(strcmp(register_pack.MAC_addr, alive_pack.MAC_addr) != 0 || 
	   strcmp(register_pack.nom_equip, alive_pack.nom_equip) != 0 ||
	   strcmp(register_pack.num_aleatori, alive_pack.num_aleatori) != 0){
		   debugger(debug, "Paquet Alive no correspon al servidor registrat, paquet ignorat ");
		   return 1;
	}else{
		return 0;
	}
}
int register_req(int debug, struct meta_struct *metastruct)
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
	if(answ != 1)
	{
		fprintf(stderr, "Register answer time expired\n");
		exit(-2);
	}
	return atoi(metastruct->recv_reg_UDP.dades);
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
    if(select(metastruct->sock+1, &fdset, NULL, NULL, &timeout) == 0){
			send_UDP_pack(debug, metastruct);
			debugger(debug, "Enviat REGISTER_REQ");
	}else{
			debugger(debug, "Rebuda resposta a REGISTER_REQ");
			metastruct->recv_reg_UDP =  recvfrom_UDP(metastruct->sock);
			answ = UDP_answer_treatment(debug, metastruct->recv_reg_UDP);
	}
    return answ;
}
int UDP_answer_treatment(int debug, struct PDU_UDP_package recv_reg_UDP) /*TODO: canviar a switch case*/
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
		fprintf(stderr,"DEBUG ==> %s\n",message);
	}
}
struct PDU_UDP_package recvfrom_UDP(int sock)
{
	struct PDU_UDP_package recv_reg_UDP;
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