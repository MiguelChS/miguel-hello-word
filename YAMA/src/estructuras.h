/*
 * estructuras.h
 *
 *  Created on: 8/9/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

typedef struct
{
	char *fs_ip;
	char *fs_puerto;
	char *algortimo_bal;
	char *yama_puerto;
	int retardo_plan;
	int socket_fs;
	int server_;

}t_configuracion;

typedef struct
{
	int master;
	int socket_;
}t_master;

typedef enum
{
	TRANSFORMACION,
	REDUCCION_LOCAL,
	REDUCCION_GLOBAL,
	ALMACENAMIENTO_FINAL,
}e_etapa;

typedef enum
{
	EN_PROCESO,
	ERROR,
	FINALIZADO_OK,
}e_estado;

typedef struct
{
	int job;
	int master;
	int nodo;
	int bloque;
	e_etapa etapa;
	char *archivo_temporal;
	e_estado estado;
}t_estado;

#endif /* ESTRUCTURAS_H_ */
