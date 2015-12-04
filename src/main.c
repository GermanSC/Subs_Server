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
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Subs_Reads.h"

int main(int argc, char *argv[])
{
	/*	Variables de control	*/

	int		ctrl	=	0;

	/*	Variables de Conexión	*/
	int		port	=	15002;
	int		sock_srv, nuevofd;
	struct	sockaddr_in	srv_addr;
	struct	sockaddr_in	client_info;

	/*	Variables del archivo	*/
	FILE* sub;
	char path[]	=	"subs.srt";
	sub		=	fopen(path,"r");

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

	int delta_sec;
	int delta_mil;
	int prev_secouot = 0;
	int prev_milout = 0;
	struct timeval tv;

	while(1)
	{
		/*	Rutina de Lectura	*/

		ctrl = leerSubs( sub, (Current_Line.Text), &(Current_Line.sec_in),
								&(Current_Line.sec_out),&(Current_Line.mill_in),
								&(Current_Line.mill_out),(Current_Line.linea));

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

		printf("%s",Current_Line.Text);

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
		printf("\n\n\n\n");
	}

	return 0;
}
