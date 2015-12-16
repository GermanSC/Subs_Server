/*
 ============================================================================
 Name        : main.c
 Project	 : Subs_Server
 Author      : German Sc.
 Version     : 0.0
 Copyright   : Completamente copyrighteado 2015
 Description : Nada.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Subs_Reads.h"

/*	Variables Globales	*/
int maxfd;
fd_set	lista_clientes;
int EOF_FLAG = 0;
int SCH_FLAG = 0;
char path[40] = "";

pthread_mutex_t lock		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t EOF_lock	= PTHREAD_MUTEX_INITIALIZER;

/*	Funcion de manejo de señal	*/
static void sigchld_hdl (int sig)
{
	int stat;
	pid_t pid = 0;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0){

	printf("Conexión finalizada en socket: %d\n", WEXITSTATUS(stat));
	pthread_mutex_lock(&lock);
		FD_CLR(WEXITSTATUS(stat), &lista_clientes);
		close(WEXITSTATUS(stat));
	pthread_mutex_unlock(&lock);
	}
	SCH_FLAG = 1;
}

void* hiloLector(void ){

	int ctrl = 0;
	int j;
	int delta_sec;
	int delta_mil;
	int prev_secouot = 0;
	int prev_milout = 0;
	struct timeval tv;

	typedef	struct	sub_struct
	{
		char linea[5];
		int sec_in;
		int sec_out;
		int mill_in;
		int mill_out;
		char Text[512];
	} sub_struct;

	sub_struct Current_Line;

	FILE* sub;
	sub		=	fopen(path, "r");
	if(sub == NULL)
	{
		printf("Error de apertura de subtitulos (Ruta especificada: %s).\n\n", path);
		pthread_mutex_lock(&EOF_lock);
		EOF_FLAG = 1;
		pthread_mutex_unlock(&EOF_lock);
		return NULL;
	}
	sleep (2);
	while(1)
	{
		/*	Rutina de Lectura	*/

		ctrl = leerSubs( sub, (Current_Line.Text), &(Current_Line.sec_in),
								&(Current_Line.sec_out),&(Current_Line.mill_in),
								&(Current_Line.mill_out),(Current_Line.linea)
								);
		if(ctrl == -10)
		{
			printf("> Fin de archivo.\n");
			pthread_mutex_lock(&lock);
			for(j = 4; j <= maxfd; j++)
			{
				if (FD_ISSET(j, &lista_clientes))
				{
					if (send(j, "SSCMD_ENDOFFILE\n", 16, 0) == -1)
					{
						printf("Error al enviar mensajes.\n\n");
						pthread_mutex_lock(&EOF_lock);
						EOF_FLAG = 1;
						pthread_mutex_unlock(&EOF_lock);
						return NULL;
					}
				}
			}
			pthread_mutex_unlock(&lock);

			pthread_mutex_lock(&EOF_lock);
			EOF_FLAG = 1;
			pthread_mutex_unlock(&EOF_lock);
			return NULL;
		}

		delta_sec = Current_Line.sec_in - prev_secouot;
		delta_mil = Current_Line.mill_in - prev_milout;
		if(delta_mil < 0)
		{
			delta_sec--;
			delta_mil += 1000;
		}

		tv.tv_sec = delta_sec;
		tv.tv_usec = delta_mil * 1000;

		/*	Espero a al tiempo de entrada del subtitulo	*/
		select(0, NULL, NULL, NULL, &tv);

		pthread_mutex_lock(&lock);
		for(j = 4; j <= maxfd; j++)
		{
			if (FD_ISSET(j, &lista_clientes) != 0)
			{
				if (send(j, Current_Line.Text, strlen(Current_Line.Text), 0) == -1)
				{
					printf("Error al enviar mensajes.\n\n");
					pthread_mutex_lock(&EOF_lock);
					EOF_FLAG = 1;
					pthread_mutex_unlock(&EOF_lock);
					return NULL;
				}
			}
		}
		pthread_mutex_unlock(&lock);

		prev_secouot	=	Current_Line.sec_out ;
		prev_milout		= 	Current_Line.mill_out;

		delta_sec = Current_Line.sec_out - Current_Line.sec_in;
		delta_mil = Current_Line.mill_out - Current_Line.mill_in;
		if(delta_mil < 0)
		{
			delta_sec--;
			delta_mil += 1000;
		}

		tv.tv_sec = delta_sec;
		tv.tv_usec = delta_mil * 1000;

		select(0, NULL, NULL, NULL, &tv);
		pthread_mutex_lock(&lock);
		for(j = 4; j <= maxfd; j++)
		{
			if (FD_ISSET(j, &lista_clientes) != 0)
			{
				if (send(j, "\n\n\n\n\n", 5, 0) == -1)
				{
					printf("Error al enviar mensajes.\n\n");
					pthread_mutex_lock(&EOF_lock);
					EOF_FLAG = 1;
					pthread_mutex_unlock(&EOF_lock);
					return NULL;
				}
			}
		}
		pthread_mutex_unlock(&lock);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	/*	Variables de control	*/

	int		ctrl	=	0;

	/*	Variables de Conexión	*/
	int		port	=	15002;
	int		sock_srv, nuevofd;
	struct	sockaddr_in	srv_addr;
	struct	sockaddr_in	client_info;
	unsigned int client_len = sizeof(struct sockaddr_in);
	char clientIP[INET_ADDRSTRLEN];

	if(argc < 2)
	{
		printf("usage: %s [ruta al archivo srt a transmitir]\n\n",argv[0]);
		return -1;
	}

	/*	Seteo el manejo de señal de hijo terminado	*/

	struct sigaction act;

	memset (&act, 0, sizeof(act));
	act.sa_handler = sigchld_hdl;

	if (sigaction(SIGCHLD, &act, 0))
	{
		printf("Error de sigaction.\n\n");
		return -1;
	}

	/*	Seteo del socket	*/

	sock_srv = socket(PF_INET, SOCK_STREAM, 0);
	if( sock_srv < 0 )	/*	Error de socket	*/
	{
		printf("Error en la apertura del socket.\n\n");
		return -1;
	}

	int yes = 1;
	setsockopt(sock_srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	srv_addr.sin_family		= AF_INET;
	srv_addr.sin_port		= htons(port);
	srv_addr.sin_addr.s_addr= 0;

	ctrl = bind(sock_srv, (struct sockaddr*)&srv_addr, sizeof (struct sockaddr_in));
	if(ctrl == -1)	/*	Error del bind	*/
	{
		printf("Error al enlazar el socket.\n\n");
		close(sock_srv);
		return -1;
	}

	printf("Esperando conexiones...\n");

	ctrl = listen(sock_srv, 10);

	/*	configuro el select	*/

	maxfd = sock_srv;
	FD_ZERO(&lista_clientes);

	strcpy(path,argv[1]);
	pthread_t hilo_Lect;
	pthread_create(&hilo_Lect, NULL, (void*)&hiloLector, NULL);

	/*	Accept timeout	*/
	struct timeval acctv;
	fd_set Listen;

	/*	Comunicacion con los hijos	*/
	pthread_mutex_lock(&EOF_lock);
	while (EOF_FLAG == 0)
	{
		pthread_mutex_unlock(&EOF_lock);
		FD_SET(sock_srv, &Listen);
		acctv.tv_sec = 1;
		select(sock_srv+1, &Listen, NULL, NULL, &acctv);
		pthread_mutex_lock(&EOF_lock);
		if(FD_ISSET(sock_srv,&Listen) && SCH_FLAG == 0 && EOF_FLAG == 0)
		{	/*	Nueva conexión	*/
			nuevofd = accept(sock_srv, (struct sockaddr *)&client_info, &client_len);
			if(nuevofd < 0)
			{
				printf("Error al aceptar la conexión.\n\n");
				close(sock_srv);
				return -1;
			}

			printf("Nueva conexión desde: %s asignada a socket: %d\n"
					,inet_ntop(AF_INET,&(client_info.sin_addr),clientIP, INET_ADDRSTRLEN)
					,nuevofd);

			pthread_mutex_lock(&lock);
			if(nuevofd > maxfd)
			{
				maxfd = nuevofd;
			}
			FD_SET(nuevofd, &lista_clientes);
			pthread_mutex_unlock(&lock);

			if(fork() == 0)
			{	/*	Procesos hijo, administran conexiones	*/
				fd_set readfd;
				FD_ZERO(&readfd);
				FD_SET(nuevofd, &readfd);
				struct timeval timeout;
				timeout.tv_sec = 4;
				timeout.tv_usec = 0;

				close(sock_srv);
				write(nuevofd, "RDY_CMD", 7);

				ctrl = 1;
				char test[13]="";
				while(ctrl > 0)
				{
					timeout.tv_sec = 4;
					timeout.tv_usec = 0;
					FD_SET(nuevofd,&readfd);
					select(nuevofd+1, &readfd, NULL, NULL, &timeout);
					if(FD_ISSET(nuevofd, &readfd) != 0 )
					{
						ctrl = read(nuevofd,test,13);
					}
					else
					{
						break;
					}
				}
				close(nuevofd);
				return nuevofd;
			}
			else
			{
				; /*	El padre solo espera conexiones	*/
			}

		}
		/*	Timeout del select	*/
		SCH_FLAG = 0;

	}
	printf("Cerrando servidor y conexiones restantes.\n");
	for(ctrl = 3; ctrl <= maxfd; ctrl++)
	{
		close(ctrl);
	}
	while ((waitpid(-1, NULL, WNOHANG)) > 0)
	{
		;
	}
	return 0;
}
