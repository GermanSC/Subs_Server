/*
 * Subs_Reads.c
 *
 *  Created on: Dec 4, 2015
 *      Author: germansc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int	leerSubs( FILE * file, char* TEXT, int* SEC_IN, int* SEC_OUT, int* MIL_IN, int* MIL_OUT,  char* LINE)
{
	char* read_ctrl;
	int hs_o, min_o, seg_o;
	int hs_i, min_i, seg_i;

	fscanf(file,"%s",LINE);
	fscanf(file,"%d%*c%d%*c%2d%*c%3d",&hs_i,&min_i,&seg_i,MIL_IN);
	fscanf(file,"%*s");
	fscanf(file,"%d%*c%d%*c%2d%*c%3d",&hs_o,&min_o,&seg_o,MIL_OUT);
	fgetc(file);
	fgetc(file);

	strcpy(TEXT,"");

	char aux_line[100] = "";
	fgets(aux_line,100,file);
	while(strcmp(aux_line,"")!=13 && read_ctrl != NULL )
	{
		strcat(TEXT,aux_line);
		read_ctrl = fgets(aux_line,100,file);
	}

	if(read_ctrl == NULL)
	{
		return -10;
	}

	/*	Calculo de segundos.	*/

	*SEC_IN = hs_i*3600+min_i*60+seg_i;
	*SEC_OUT = hs_o*3600+min_o*60+seg_o;

	/*	Fin de Lectura	de un subtitulo	*/
	return 0;
}
