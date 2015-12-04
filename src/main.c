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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*	Funcion de manejo de señal	*/
static void sigchld_hdl (int sig)
{
	int stat;
	pid_t pid;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0){

	printf("Conexión finalizada en socket: %d\n",WEXITSTATUS(stat));
	pthread_mutex_lock(&lock);
		FD_CLR(WEXITSTATUS(stat),&lista_clientes);
		close(WEXITSTATUS(stat));
	pthread_mutex_unlock(&lock);
	}
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
	char path[]	=	"subs.srt";
	sub		=	fopen(path,"r");
	if(sub == NULL)
	{
		printf("Error de apertura de subtitulos.\n");
		return NULL;
	}

	while(1)
	{
		/*	Rutina de Lectura	*/

		ctrl = leerSubs( sub, (Current_Line.Text), &(Current_Line.sec_in),
								&(Current_Line.sec_out),&(Current_Line.mill_in),
								&(Current_Line.mill_out),(Current_Line.linea));
		if(ctrl != 0)
		{
			printf("  Error de Lectura de subtitulos.\n");
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
		select(0,NULL,NULL,NULL,&tv);

		pthread_mutex_lock(&lock);
		for(j = 4; j <= maxfd; j++)
		{
			if (FD_ISSET(j, &lista_clientes))
			{
				if (send(j, Current_Line.Text, strlen(Current_Line.Text), 0) == -1)
				{
					printf("Error al enviar mensajes.\n");
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

		select(0,NULL,NULL,NULL,&tv);
		pthread_mutex_lock(&lock);
		for(j = 4; j <= maxfd; j++)
		{
			if (FD_ISSET(j, &lista_clientes))
			{
				if (send(j,"\n\n\n\n\n", 5, 0) == -1)
				{
					printf("Error al enviar mensajes.\n");
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

	/*	Seteo el manejo de señal de hijo terminado	*/

	struct sigaction act;

	memset (&act, 0, sizeof(act));
	act.sa_handler = sigchld_hdl;
	act.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &act, 0))
	{
		perror ("sigaction");
		return 1;
	}

	/*	Seteo del socket	*/

	sock_srv = socket(PF_INET,SOCK_STREAM,0);
	if( sock_srv < 0 )	/*	Error de socket	*/
	{
		printf("Error en lla apertura del socket.\n\n");
		return -1;
	}

	int yes = 1;
	setsockopt(sock_srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	srv_addr.sin_family		= AF_INET;
	srv_addr.sin_port		= htons(port);
	srv_addr.sin_addr.s_addr	= 0;

	ctrl = bind(sock_srv,(struct sockaddr*)&srv_addr,sizeof (struct sockaddr_in));
	if(ctrl == -1)	/*	Error del bind	*/
	{
		printf("Error al enlazar el socket.\n\n");
		close(sock_srv);
		return -1;
	}

	printf("Esperando conexiones...\n");

	ctrl = listen(sock_srv,10);

	/*	configuro el select	*/

	maxfd = sock_srv;
	FD_ZERO(&lista_clientes);

	pthread_t hilo_Lect;
	pthread_create(&hilo_Lect,NULL,(void*)&hiloLector,NULL);

	while (1)
	{
		nuevofd = accept(sock_srv,(struct sockaddr *)&client_info,&client_len);
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

		FD_SET(nuevofd,&lista_clientes);
		pthread_mutex_unlock(&lock);

		if(fork() == 0)
		{	/*	Procesos hijo, administran conexiones	*/
			fd_set readfd;
			FD_ZERO(&readfd);
			FD_SET(nuevofd,&readfd);
			struct timeval timeout;
			timeout.tv_sec = 4;
			timeout.tv_usec = 0;

			close(sock_srv);
			write(nuevofd,"RDY_CMD",7);
			sleep(1);
			ctrl = 1;
			while(ctrl > 0)
			{
				timeout.tv_sec = 4;
				timeout.tv_usec = 0;
				FD_SET(nuevofd,&readfd);
				select(nuevofd+1,&readfd,NULL,NULL,&timeout);
				if(FD_ISSET(nuevofd,&readfd))
				{
					char test[15]="";
					ctrl = read(nuevofd,test,15);
				}
				else
				{
					break;
				}

			}
			return nuevofd;
		}
		else
		{

		}

	}

	return 0;
}
