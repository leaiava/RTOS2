/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

#ifndef separacion_frames_H_
#define separacion_frames_H_

#include "sapi.h"
#include "crc8.h"
#include "objeto.h"
#include "qmpool.h"

typedef struct
{
	uartMap_t	uart;		///< Nombre de la UART del LPC4337 a utilizar
	uint32_t 	baudRate;	///< BaudRate seleccionado para la comunicacion serie
	uint8_t*	buffer;		///< buffer para la recepción de bytes
	uint32_t	cantidad;	///< cantidad de bytes recibidos
	bool		SOM;		///< flag para indicar si llego el SOM
	bool		EOM;		///< flag para indicar si llego el EOM
	SemaphoreHandle_t sem_ISR;	///< Semaforo que la ISR usa para indicarle a la tarea que tiene un paquete listo.
	SemaphoreHandle_t sem_bloque_liberado; ///< Semaforo que le indica al driver que se libero un bloque de memoria.
	tObjeto*	ptr_objeto1;	///< ptr al objeto usado para enviar el mensaje del driver a la aplicacion
	tObjeto*	ptr_objeto2;	///< ptr al objeto usado para enviar el mensaje de la aplicacion al driver
	tMensaje* 	ptr_mensaje;///< ptr al mensaje a enviarse a través del objeto.
}sf_t;

sf_t* sf_crear(void);
bool sf_init(sf_t* handler,uartMap_t uart, uint32_t baudRate);

void sf_mensaje_recibir( sf_t* handler );
void sf_mensajeProcesado_enviar( sf_t* handler );

#endif /* separacion_frames_H_ */
