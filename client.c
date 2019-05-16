#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/select.h>
#include <sys/wait.h>
#include <getopt.h>
#include <errno.h>


#define h_addr h_addr_list[0] /* Backward compatibility */

/* --- Timeout defines --- */
#define n 3
#define t 2
#define m 4
#define p 8
#define s 5
#define q 3
#define r 3
#define u 3
#define w 4

/* --- Estructures --- */

/* Estructura on es guarden les dades de configuració de client.cfg 
   (nom per defecte) */
struct cfg_data{
	char nom_equip[7];
	char MAC_equip[13];
	char nom_server[25];
	int port_server;
};

/* Estructura de la PDU per la comunicació UDP */
struct PDU_UDP_package{
	unsigned char tipus_paquet;
	char nom_equip[7];
	char MAC_addr[13];
	char num_aleatori[7];
	char dades[50];
};

/* Estructura de la PDU per la comunicació TCP*/
struct PDU_TCP_package{
	unsigned char tipus_paquet;
	char nom_equip[7];
	char MAC_addr[13];
	char num_aleatori[7];
	char dades[150];
};

<<<<<<< HEAD
/* Estructura que serveix per guardar estructures i elements de frequent ús 
=======
/* Estructura que serveix per guardar estructures de frequent ús 
>>>>>>> 6036907bc1af0ddcd340e611ed92393efd48cf53
   per evitar capçaleres de funcions excessivament llargues */
struct meta_struct{
	int sock;
	struct sockaddr_in addr_server;
	struct PDU_UDP_package tosend_UDP_pack, recv_reg_UDP;
};

/* Estructura amb les dades necessàres per la consola (per passar-li a l'hora
   del fork)*/
struct console_needs{
	int fdcons[2];
	int fdcli[2];
	int tcp_port;
	char boot_file[20];
	struct PDU_UDP_package reg_info;
	in_addr_t  s_addr;
};


/* Declaració de funcions */
struct cfg_data collect_config_data(char cfg_file[]);
char* change_cfg_filename(int argc, char *argv[]);
void fill_structures(struct cfg_data dataconfig, struct hostent *ent, 
							  struct meta_struct *metastruct);
void send_UDP_pack(int debug, struct meta_struct *metastruct);
struct PDU_UDP_package recvfrom_UDP(int sock);
int register_req(int debug,struct meta_struct *metastruct);
int register_process(fd_set fdset, struct timeval timeout, int debug,
                    struct meta_struct *metastruct);	
int UDP_answer_treatment(int debug, struct PDU_UDP_package recv_reg_UDP, int sock);
void debugger(int debug, char message[]);
int select_process(int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct);
void alive(int debug ,struct meta_struct *metastruct, struct console_needs *console_struct);
int authenticate_alive(int debug, struct PDU_UDP_package register_pack,
					   struct PDU_UDP_package alive_pack);
void console(int debug, struct console_needs console_struct);
void fork_console(int debug, struct console_needs *console_struct);
void check_console_command(int sock, int fdcons[2]);
void close_commandline(int fdcli[2]);
void sendconf(int debug, struct console_needs console_struct);
void pass_info(struct console_needs *console_struct, struct meta_struct metastruct);
void send_data(int debug, int sockfd, struct PDU_TCP_package send_tcp_pack, FILE* fp);
void connect_TCP(int debug, int sockfd, struct console_needs console_struct);
void select_sendconf(int debug, int sockfd, struct PDU_TCP_package recv_tcp_pack, 
					 struct PDU_TCP_package send_tcp_pack, FILE* fp);
void getconf(int debug, struct console_needs console_struct);
void get_and_save_conf(int debug, int sockfd, FILE* fp);



/* Funcó principal */
int main(int argc, char *argv[])
{
	char *cfg_file = "client.cfg";
	char *boot_file = "boot.cfg";
	int debug = 0;
	int option;

    struct meta_struct metastruct;
	struct cfg_data dataconfig;
	struct hostent *ent;
	struct console_needs console_struct;
    ent = malloc(sizeof(ent));

    memset(&metastruct,0,sizeof (struct meta_struct));

	/* While que tracta amb getopt els arguments passats per paràmetre */
	while((option = getopt(argc, argv, "dc:f:")) != -1){
		switch (option)
		{
			case 'c':
				cfg_file = optarg;
				break;
			case 'f':
				boot_file = optarg;
				break;
			case 'd':
				debug = 1;
				break;
			default:
				fprintf(stderr, "Argument incorrecte");
				exit(1);
		}
	}

	fprintf(stderr, "ESTAT: DISCONNECTED\n");
	debugger(debug, "Collecting configuration data");

	dataconfig = collect_config_data(cfg_file);

	strcpy(metastruct.tosend_UDP_pack.nom_equip,dataconfig.nom_equip); 
	strcpy(console_struct.boot_file, boot_file);


	debugger(debug, "Opening UDP socket");
	/* Obre socket UDP */
	if((metastruct.sock = socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("Error al obrir el socket UDP:");
		exit(-1);
	}

	fill_structures(dataconfig, ent, &metastruct);
	
	while(1){
		console_struct.tcp_port = register_req(debug, &metastruct);
		alive(debug, &metastruct, &console_struct);
	}

}
/* Envia "close" al procés fill per a que es tanqui */
void close_commandline(int fdcli[2])
{
	write(fdcli[1],"close",sizeof("close"));
}

/* Amb l'ajuda d'un select (per a que no sigui bloquejant), mira si s'ha rebut
   informació des del procés de la consola. Si la informació obtinguda és "quit",
   procedeix a tancar el socket i el procés */
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
	if(FD_ISSET(fdcons[0], &fdset))
	{
		read(fdcons[0], buff, sizeof(buff));
		if(strcmp(buff,"quit"))
		{
			close(sock);
			exit(1);
		}	
	}
}

/* Crea 2 pipes i crea un fill mitjançant la crida fork(). fdcons és el descriptor
   de fitxer que utilitzarà el procés fill per enviar dades al pare, i fdcli és 
   el que utilitzarà el pare per enviar dades al fill*/
void fork_console(int debug, struct console_needs *console_struct)
{
	pid_t pid;
	if(pipe(console_struct->fdcons) == -1 || pipe(console_struct->fdcli) == -1)
	{
		fprintf(stderr,"Pipe error"); 
		exit(1);
	}

	pid = fork();	
	
	if (pid == 0)
	{
		close(console_struct->fdcons[0]);
		close(console_struct->fdcli[1]);
		console(debug, *console_struct);
	}
	else if(pid != -1)
	{	
		close(console_struct->fdcons[1]);
		close(console_struct->fdcli[0]);
	}
	else
	{
		fprintf(stderr,"Error amb el fork del procés de la consola");
		exit(1);
	}
}

/* Mètode que implementa totes les funcionalitats de la consola, és un bucle
   on, mitjançant un select, controla el flux de dades tant del pipe (dades
   que li envia el procés pare), com per stdin (comandes de l'usuari) */
void console(int debug, struct console_needs console_struct)
{
	char buff[20];
	struct timeval timeout;
	fd_set fdset;
	fprintf(stderr,"consola oberta!\n");

	while(1)
	{
    	FD_ZERO(&fdset);
    	FD_SET(console_struct.fdcli[0], &fdset);
		FD_SET(0, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	select(console_struct.fdcli[0]+1, &fdset, NULL, NULL, &timeout);
		if(FD_ISSET(console_struct.fdcli[0],&fdset))
		{
			read(console_struct.fdcli[0], buff, sizeof(buff));
			if(strcmp(buff,"close\n") || strcmp(buff,"close"))
			{
				fprintf(stderr,"Tancant consola\n");
				exit(1);
			}
		}
		if(FD_ISSET(0,&fdset))
		{
			fgets(buff, sizeof(buff), stdin);
			if(strcmp(buff,"quit\n")==0 || strcmp(buff,"quit") == 0)
			{
				write(console_struct.fdcons[1],buff,4);
				fprintf(stderr, "Tancant consola\n");
				fprintf(stderr, "El client es tancarà en uns instants\n");
				exit(1);
			}
			else if(strcmp(buff, "send-conf\n")== 0 || strcmp(buff, "send-conf") == 0)
			{
				debugger(debug, "Executant comanda d'enviament de configuració");				
				sendconf(debug, console_struct);
			}
			else if(strcmp(buff, "get-conf\n") == 0 || strcmp(buff, "get-conf") == 0)
			{
				debugger(debug, "Executant comanda d'obtenció de configuració");
				getconf(debug, console_struct);
			}
			else if(strcmp(buff, "\n") != 0)
			{
				fprintf(stderr, "Comanda incorrecta\n");
			}
			fprintf(stderr, ">");
		}
	}
}

/* Mètode que envia el paquet d'obtenció de configuració, tracta la resposta del
   servidor, i afegeix les dades de configuració al fitxer pertinent */
void getconf(int debug, struct console_needs console_struct)
{
	struct PDU_TCP_package send_tcp_pack;
	FILE *fp;
	int sz;
	int sockfd;
	
	if((fp = fopen(console_struct.boot_file, "w")) == NULL)
	{
		fprintf(stderr, "No s'ha pogut obrir el fitxer de configuració %s\n",
			    console_struct.boot_file);
		exit(-1);
	}
	
	send_tcp_pack.tipus_paquet = 0x30;
	strcpy(send_tcp_pack.MAC_addr,console_struct.reg_info.MAC_addr);
	strcpy(send_tcp_pack.nom_equip,console_struct.reg_info.nom_equip);
	strcpy(send_tcp_pack.num_aleatori,console_struct.reg_info.num_aleatori);
	memset(send_tcp_pack.dades, 0, sizeof(send_tcp_pack.dades));
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	rewind(fp);
	sprintf(send_tcp_pack.dades, "%s,%d", console_struct.boot_file, sz);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "Error al obrir el socket");
		exit(-1);
	} 

	connect_TCP(debug, sockfd, console_struct);


	send(sockfd, &send_tcp_pack, sizeof(send_tcp_pack),0);

	get_and_save_conf(debug, sockfd, fp);

	debugger(debug, "Tancant canal TCP...");
	fclose(fp);
	close(sockfd);
}

/* Bucle que controla la cadència de dades rebudes pel socket TCP i col·loca les
   dades a l'arxiu de configuració correcte */
void get_and_save_conf(int debug, int sockfd, FILE* fp)
{
	struct PDU_TCP_package recv_tcp_pack;
	struct timeval timeout;
	fd_set fdset;
	int end = 0;
	
	while(end == 0)
	{
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);
		timeout.tv_sec = w;
		timeout.tv_usec = 0;
		if(select(sockfd+1, &fdset, NULL, NULL, &timeout) == 0)
		{
			debugger(debug, "Temps d'espera de GET_CONF vençut");
			end = 1;
		}
		else
		{
			if(recv(sockfd, &recv_tcp_pack, sizeof(recv_tcp_pack), 0) < 0)
			{
				fprintf(stderr, "Error al rebre paquet TCP\n");
				close(sockfd);
				exit(-1);		
			}
			if(recv_tcp_pack.tipus_paquet == 0x31)
			{
				debugger(debug, "Rebut GET_ACK");

			}
			else if(recv_tcp_pack.tipus_paquet == 0x34)
			{
				debugger(debug, "Rebuda línia de l'arxiu configuració");
				fprintf (fp, recv_tcp_pack.dades);
			}
			else if(recv_tcp_pack.tipus_paquet == 0x35)
			{
				debugger(debug, "Rebut GET_END");
				end = 1;
			}
			else
			{
				debugger(debug, "Paquet GET_FILE rebutjat, motiu:");
				debugger(debug, recv_tcp_pack.dades);
				end = 1;
			}
		}
	}
}

/* Mètode que implementa l'enviament de configuració al servidor */
void sendconf(int debug, struct console_needs console_struct)
{
	struct PDU_TCP_package send_tcp_pack;
	struct PDU_TCP_package recv_tcp_pack;
	FILE *fp;
	int sz;
	int sockfd;

	if((fp = fopen(console_struct.boot_file, "r")) == NULL)
	{
		fprintf(stderr, "No s'ha pogut obrir el fitxer de configuració %s\n",
	            console_struct.boot_file);
		exit(-1);
	}
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	rewind(fp);
	sprintf(send_tcp_pack.dades, "%s,%d", console_struct.boot_file, sz);

	send_tcp_pack.tipus_paquet = 0x20;
	strcpy(send_tcp_pack.MAC_addr,console_struct.reg_info.MAC_addr);
	strcpy(send_tcp_pack.nom_equip,console_struct.reg_info.nom_equip);
	strcpy(send_tcp_pack.num_aleatori,console_struct.reg_info.num_aleatori);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "Error al obrir el socket");
		exit(-1);
	} 

	connect_TCP(debug, sockfd, console_struct);

	send(sockfd, &send_tcp_pack, sizeof(send_tcp_pack),0);
	debugger(debug, "Enviat SEND_FILE");


	select_sendconf(debug, sockfd, recv_tcp_pack, send_tcp_pack, fp);
	
	debugger(debug, "Tancant canal TCP...");
	close(sockfd);
	fclose(fp);
}

/* Funció que implementa la part del select de l'enviament de configuració, si 
   es supera el temps d'espera estipulat no segueix amb la transacció */
void select_sendconf(int debug, int sockfd, struct PDU_TCP_package recv_tcp_pack, 
					 struct PDU_TCP_package send_tcp_pack, FILE* fp)
{
	fd_set fdset;
	struct timeval timeout;

	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	timeout.tv_sec = w;
	timeout.tv_usec = 0;
	if(select(sockfd+1, &fdset, NULL, NULL, &timeout) == 0)
	{
		debugger(debug, "Temps d'espera de SEND_FILE vençut");		
	}
	else
	{
		if(recv(sockfd, &recv_tcp_pack, sizeof(recv_tcp_pack), 0) < 0)
		{
			fprintf(stderr, "Error al enviar paquet TCP\n");
			close(sockfd);
			exit(-1);
		}
		if(recv_tcp_pack.tipus_paquet == 0x21)
		{
			debugger(debug, "Rebut SEND_ACK");
			send_data(debug, sockfd, send_tcp_pack, fp);
		}
		else
		{			
			debugger(debug, "Paquet SEND_FILE rebutjat, motiu:");
			debugger(debug, recv_tcp_pack.dades);
		}
	}
}

/* Efectua la connexió TCP amb el servidor */
void connect_TCP(int debug, int sockfd, struct console_needs console_struct)
{
	struct sockaddr_in addr_server;
	addr_server.sin_family = AF_INET;
	addr_server.sin_addr.s_addr = console_struct.s_addr;
	addr_server.sin_port = htons(console_struct.tcp_port);
	if (connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server)) != 0)
	{ 
    	fprintf(stderr,"La connexió amb el servidor ha fallat\n"); 
    	exit(0); 
    } 
	debugger(debug, "Connectant socket TCP");
}

/* Mentres hi hagi línies al fitxer de configuració, les envia una a una pel socket
   TCP. Per últim, envia el SEND_END */
void send_data(int debug, int sockfd, struct PDU_TCP_package send_tcp_pack, FILE* fp)
{
	debugger(debug, "S'ha començat a enviar les línies del arxiu de configuració");
	send_tcp_pack.tipus_paquet = 0x24;
	while(fgets(send_tcp_pack.dades, sizeof(send_tcp_pack.dades), fp) != NULL)
	{
		send(sockfd, &send_tcp_pack, sizeof(send_tcp_pack),0);
	}
	debugger(debug, "Enviades totes les línies de l'arxiu de configuració");
	debugger(debug, "Enviant SEND_END");
	memset(send_tcp_pack.dades, 0,sizeof(send_tcp_pack.dades));
	send_tcp_pack.tipus_paquet = 0x25;
	send(sockfd, &send_tcp_pack, sizeof(send_tcp_pack), 0);
}

/* Funció que s'encarrega de tota la part d'alve del client*/
void alive(int debug, struct meta_struct *metastruct, struct console_needs *console_struct)
{
	int intent = 0; 
	int first = 1;
	fd_set fdset;
	struct timeval timeout;
	struct PDU_UDP_package recv_alive_UDP;
	metastruct->tosend_UDP_pack.tipus_paquet = 0x10;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,
					metastruct->recv_reg_UDP.num_aleatori);
	while(intent < u)
	{
		debugger(debug, "Enviat ALIVE_INF");
		send_UDP_pack(debug, metastruct);
		sleep(r);
		if(!first)
		{
			check_console_command(metastruct->sock, console_struct->fdcons);
		}
		FD_ZERO(&fdset);
    	FD_SET(metastruct->sock, &fdset);
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
    	if(select(metastruct->sock+1, &fdset, NULL, NULL, &timeout) == 0)
		{
			debugger(debug, "No s'ha rebut resposta a ALIVE_INF");
			intent++;
		}
		else
		{
			recv_alive_UDP = recvfrom_UDP(metastruct->sock);
			if(UDP_answer_treatment(debug, recv_alive_UDP, metastruct->sock) == 1 && 
			   authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0)
			{
				intent = 0;
				if(first == 1)
				{
					pass_info(console_struct,*metastruct);
					fork_console(debug, console_struct);
					debugger(debug, "Rebut primer alive!");
					fprintf(stderr, "ESTAT: ALIVE\n");
				}
				first = 0;
			}
			else if(UDP_answer_treatment(debug, recv_alive_UDP, metastruct->sock) == 2 &&
					 authenticate_alive(debug, metastruct->recv_reg_UDP,recv_alive_UDP) == 0)
			{
				intent = u;
				debugger(debug, "Rebut ALIVE_REJ correcte, intent de suplantació d'indentitat");
				fprintf(stderr, "ESTAT: DISCONNECTED");
			}
			else
			{
				intent++;
			}
		}
	}
	if(!first)
	{
		debugger(debug, "Temps d'espera d'alives ha expirat");
		fprintf(stderr, "ESTAT: DISCONNECTED\n");
		close_commandline(console_struct->fdcli);
	}
}

/* Passa informació necessària a l'estructura de la consola (per fer el fork)*/
void pass_info(struct console_needs *console_struct, struct meta_struct metastruct){
	strcpy(console_struct->reg_info.MAC_addr,metastruct.tosend_UDP_pack.MAC_addr);
	strcpy(console_struct->reg_info.nom_equip,metastruct.tosend_UDP_pack.nom_equip);
	strcpy(console_struct->reg_info.num_aleatori,metastruct.tosend_UDP_pack.num_aleatori);
	
	console_struct->s_addr = metastruct.addr_server.sin_addr.s_addr;
}
/* Donat el paquet UDP obtingut al registre, i un paquet UDP obtingut en l'alive, retorna
   1 si el paquet alive no correspón (no està autoritzat) al registrat, i 0 altrament */
int authenticate_alive(int debug,struct PDU_UDP_package register_pack,
					   struct PDU_UDP_package alive_pack)
{
	debugger(debug, "Autenticant el paquet Alive rebut");
	if(strcmp(register_pack.MAC_addr, alive_pack.MAC_addr) != 0 || 
		strcmp(register_pack.nom_equip, alive_pack.nom_equip) != 0 ||
		strcmp(register_pack.num_aleatori, alive_pack.num_aleatori) != 0)
	{
		debugger(debug, "Paquet Alive no correspon al servidor registrat, paquet ignorat ");
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Funció que s'encarrega de tot el registre del client */
int register_req(int debug, struct meta_struct *metastruct)
{
	int intent;
	int answ = 0;
	struct timeval timeout;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(metastruct->sock, &fdset);
	timeout.tv_usec = 0;
	metastruct->tosend_UDP_pack.tipus_paquet = 0x00;
	strcpy(metastruct->tosend_UDP_pack.num_aleatori,"000000");

	
	for(intent = 0; intent<(q) && answ != 1; intent++)
	{
		debugger(debug, "Començant procés de registre");
		answ = register_process(fdset, timeout, debug, metastruct);
	}
	if(answ != 1)
	{
		fprintf(stderr, "Register answer time expired\n");
		fprintf(stderr, "ESTAT: DISCONNECTED\n");
		close(metastruct->sock);
		exit(-2);
	}
	fprintf(stderr, "ESTAT: REGISTERED\n");
	return atoi(metastruct->recv_reg_UDP.dades);
}

/* Funció que efectua un procés de registre, retorna un sencer en funció de la
   resposta retornada per el mètode select_process */
int register_process(fd_set fdset, struct timeval timeout, int debug,
                    struct meta_struct *metastruct)
{
	int intent;
	int h = 1;
	int answ = 0;
	timeout.tv_sec = t;
	send_UDP_pack(debug, metastruct);
	debugger(debug, "Enviat REGISTER_REQ");
	fprintf(stderr, "ESTAT: WAIT_REG\n");
	for(intent = 1; intent<p && answ == 0;intent++)
	{
		if(intent>=n && (intent-n)<m-1)
		{
			h++;
		}
		timeout.tv_sec = h*t;
    	answ = select_process(debug, fdset, timeout, metastruct);
	}
	if(answ != 0)
	{
		return answ;
	}
	else
	{
		timeout.tv_sec = s;
		answ = select_process(debug, fdset, timeout, metastruct);
		return answ;
	}
}

/* Espera la recepeció d'un paquet per UDP el temps establert (per timeout) amb select, 
   si no es rep res, torna a enviar una petició de registre, si rep un paquet
   el tracta amb la funció UDP_answer_treatment, i retorna un sencer per a que la funció
   que l'ha cridat sapigui quina acció fer */
int select_process(int debug, fd_set fdset, struct timeval timeout,
                     struct meta_struct *metastruct)
{
    int answ = 0;
    FD_ZERO(&fdset);
    FD_SET(metastruct->sock, &fdset);
    if(select(metastruct->sock+1, &fdset, NULL, NULL, &timeout) == 0){
		send_UDP_pack(debug, metastruct);
		debugger(debug, "Enviat REGISTER_REQ");
	}
	else
	{
		debugger(debug, "Rebuda resposta a REGISTER_REQ");
		metastruct->recv_reg_UDP =  recvfrom_UDP(metastruct->sock);
		answ = UDP_answer_treatment(debug, metastruct->recv_reg_UDP, metastruct->sock);
	}
    return answ;
}

/* Se li passa per paràmetre un paquet rebut per UDP. La funció, depenent del tipus de 
   paquet, retorna un sencer amb el que la funció que l'ha cridat podrà saber quia acció
   ha d'efectuar  */
int UDP_answer_treatment(int debug, struct PDU_UDP_package recv_reg_UDP, int sock)
{
	switch (recv_reg_UDP.tipus_paquet)
	{
	case 0x01:
		debugger(debug, "Paquet rebut, REGISTER_ACK");
		debugger(debug, "ESTAT: REGISTERED");
		return 1;
		break;
	case 0x02:
		debugger(debug, "Paquet rebut, REGISTER_NACK");
		return 2;
		break;
	case 0x03:
		debugger(debug, "Paquet rebut, REGISTER_REJ");
		debugger(debug, "ESTAT: DISCONNECTED");
		fprintf(stderr,"El registre ha estat rebutjat. Motiu: %s\n",recv_reg_UDP.dades);
		close(sock);
		exit(-1);
		break;
	case 0x09:
		debugger(debug, "Paquet rebut, ERROR");
		debugger(debug, "ESTAT: DISCONNECTED");
		debugger(debug,"Error de protocol");
		close(sock);
		exit(-2);
		break;
	case 0x11:
		debugger(debug, "Paquet rebut, ALIVE_ACK");
		return 1;
		break;
	case 0x12:
		debugger(debug, "Paquet rebut, ALIVE_NACK");
		return 0;
		break;
	case 0x13:
		debugger(debug, "Paquet rebut, ALIVE_REJ");
		return 2;
		break;
	default:
		debugger(debug,"Paquet rebut, NO IDENTIFICAT");
		close(sock);
		exit(-2);
		break;
	}
}
/* Funció per mostrar per pantalla els missatges de debug (passats per paràmetre)*/
void debugger(int debug, char message[])
{
	if(debug == 1)
	{
		fprintf(stderr,"DEBUG ==> %s\n",message);
	}
}

/* Llegeix el paquet pel canal UDP i el retorna */
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

/* Envia el paquet a enviar (tosend_UDP_pack) pel canal UDP a l'adreça del servidor */
void send_UDP_pack(int debug, struct meta_struct *metastruct)
{
	if(sendto(metastruct->sock, &metastruct->tosend_UDP_pack,
		sizeof(metastruct->tosend_UDP_pack),0, 
		(struct sockaddr*) &metastruct->addr_server, sizeof(metastruct->addr_server)) < 0)
	{
		perror("Error al enviar el paquet");
		exit(-1);
	}
}

/* Omple l'estructura de l'adreça del servidor i l'estructura del paquet UDP a enviar */
void fill_structures(struct cfg_data dataconfig, struct hostent *ent, 
							  struct meta_struct *metastruct)
{
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

/* Agafa la informació de l'arxiu de configuració per ficar-la en la estructura pertinent
   i retornar aquesta */
struct cfg_data collect_config_data(char cfg_file[])
{
	FILE *fpointer;
    char singleLine[100];
    char *param_name;
	struct cfg_data dataconfig;

	if((fpointer = fopen(cfg_file,"r")) == NULL)
	{
		fprintf(stderr, "File does not exist\n");
		exit(1);
	}
    while(fgets(singleLine,100,fpointer) != NULL)
    {
        param_name = strtok(singleLine," ");             
        if(strcmp(param_name,"Nom") == 0)
        {
            strcpy(dataconfig.nom_equip, strtok(NULL,"\n"));
	    }
        else if(strcmp(param_name,"MAC") == 0)
        {
            strcpy(dataconfig.MAC_equip,strtok(NULL,"\n"));
        }
        else if(strcmp(param_name,"Server") == 0)
        {
            strcpy(dataconfig.nom_server,strtok(NULL,"\n"));
        }
        else if(strcmp(param_name,"Server-port") == 0)
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
