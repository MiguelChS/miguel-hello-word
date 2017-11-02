#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <funcionesCompartidas/funcionesNet.h>
#include <funcionesCompartidas/log.h>
#include <funcionesCompartidas/mensaje.h>
#include <funcionesCompartidas/estructuras.h>
#include <funcionesCompartidas/serializacion_yama_master.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "conexion_master.h"
#include "conexion_fs.h"
#include "manejo_tabla_estados.h"
#include "clocks.h"

extern t_configuracion *config;
extern t_log *yama_log;
extern t_list *masters;
extern int master_id;
fd_set master;
fd_set read_fds;
int fdmax;

void manejo_conexiones()
{
	int controlador = 0;

	escribir_log(yama_log, "Iniciando administrador de conexiones Master");
	//Seteo en 0 el master y temporal
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//Cargo el socket server
	FD_SET(config->server_, &master);

	//Cargo el socket mas grande
	fdmax = config->server_;

	//Bucle principal
	while (1)
	{
		read_fds = master;

		int selectResult = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
		escribir_log(yama_log, "Actividad detectad<a en administrador de conexiones");

		if (selectResult == -1)
		{
			break;
			escribir_error_log(yama_log, "Error en el administrador de conexiones");
		}
		else
		{
			//Recorro los descriptores para ver quien llamo
			int i;
			for (i = 0; i <= fdmax; i++)
			{
				if (FD_ISSET(i, &read_fds))
				{
					//Se detecta alguien nuevo llamando?
					if (i == config->server_)
					{
						//Gestiono la conexion entrante
						escribir_log(yama_log, "Se detecto actividad en el server Master");
						int nuevo_socket = aceptar_conexion(config->server_, yama_log, &controlador);

						//Controlo que no haya pasado nada raro y acepto al nuevo
						if (controlador == 0)
						{
							int id_m = master_id++;
							list_add(masters, &id_m);
							//realizar_handshake_master(nuevo_socket);
						}

						//Cargo la nueva conexion a la lista y actualizo el maximo
						FD_SET(nuevo_socket, &master);

						if (nuevo_socket > fdmax)
						{
							fdmax = nuevo_socket;
						}
					}
					else
					{
						manejar_respuesta(i);
					}
				}
			}
		}
	}
}


void manejar_respuesta(int socket_)
{
	header head;
	int status;
	char *mensaje = (char *)getMessage(socket_, &head, &status);
	//char *header = get_header(mensaje);
	if (head.letra == 'M')
	{
		//int codigo = get_codigo(mensaje);
		//char *info = get_mensaje(mensaje);
		switch (head.codigo)
		{
			case 0:; //procesar archivo -> peticion transformacion
				escribir_log(yama_log, "Se recibió archivo para procesar");
				solicitar_informacion_archivo(mensaje, socket_); //aca file me debería devolver algo
				//enviar_peticion_transformacion(socket_);
				break;
			case 5:; //reduccion local
				escribir_log(yama_log,"Se recibió el estado de una transformacion");
				t_estado_master *estado_tr = deserializar_estado_master(mensaje);
				if(estado_tr->estado == 2)
				{
					enviar_reduccion_local(estado_tr, socket_);
				}else; //replanificar
				//preguntar que necesita que le mande cuando no está lista la reduccion local
				break;
			case 6:;
				t_estado_master *estado_tr2 = deserializar_estado_master(mensaje);
				if(estado_tr2->estado == 2)
				{
						t_worker *wk = find_worker(estado_tr2->nodo);
						t_redGlobal *red = malloc(sizeof(t_redGlobal));
						red->encargado = 1;
						red->temp_red_local = "prueba2";
						red->nodo = wk->nodo;
						red->red_global = "prueba5";

						t_list *listita = list_create();

						list_add(listita, red);
						size_t len = 0;
						char *red_ser = serializar_lista_redGlobal(listita, &len);

						header head;
						head.codigo = 3;
						head.letra = 'Y';
						head.sizeData = len;

						int cont;

						message *men = createMessage(&head, red_ser);
						enviar_message(socket_,men,yama_log, &cont);
				}
				break;
			case 7:;
				break;
			case 8:;
				break;
			default:
				printf("default");
				break;
		}
	} else log_error(yama_log, "Mensaje de emisor desconocido");
	free(mensaje);
}

void realizar_handshake_master(int socket_)
{
	int control;
	enviar(socket_, "Y000000000000000", yama_log, &control);
	manejar_respuesta(socket_);
	t_master *master = malloc (sizeof (t_master));
	master->master = master_id ++;
	master->socket_ = socket_;
	list_add(masters, master);
}

void enviar_peticion_transformacion(int socket_)
{
	t_nodo *nodo;
	t_transformacion *transformacion;
	header *head;
	message *mensaje;
	int control = 0;

	nodo = malloc(sizeof(t_nodo));
	nodo->ip = strdup("127.0.0.1");
	nodo->nodo = strdup("Nodo 1");
	nodo->puerto = 5002;

	transformacion = malloc(sizeof(t_transformacion));
	transformacion->bloque = 18;
	transformacion->bytes = 1024;
	transformacion->nodo = nodo;
	transformacion->temporal = strdup("/archivo/temporal_prueba");

	size_t size_buffer = 0;

	char *t_ser = serializar_transformacion(transformacion, &size_buffer);

	head = malloc(sizeof(header));
	head->codigo = 1;
	head->letra = 'Y';
	head->sizeData = size_buffer;

	mensaje = createMessage(head, t_ser);
	enviar_message(socket_, mensaje, yama_log, &control);

	//agregar frees
	free(head);
}
