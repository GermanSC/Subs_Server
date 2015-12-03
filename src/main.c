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
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	/*	Variables de Conexión	*/
	int		port		=	15001;
	int		sock_srv, nuevofd;
	struct sockaddr_in	server_sock;
	struct sockaddr_in	client_info;

	return 0;
}
