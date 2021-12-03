/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 12/11/2021
 * Version: v2.0
 *===========================================================================*/

#ifndef OBJETO_H_
#define OBJETO_H_

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"
#include <string.h>
#include "FreeRTOSConfig.h"

/*==================[definiciones y macros]==================================*/
#define N_QUEUE	10
#define PAQUETE     1
#define RESPUESTA   2

typedef struct
{
	uint32_t evento_tipo;
	uint32_t cantidad;
	uint8_t* ptr_datos;
}tMensaje;

typedef struct
{
    QueueHandle_t cola;
} tObjeto;

tObjeto* objeto_crear();
void objeto_post( tObjeto* objeto,tMensaje mensaje );
void objeto_post_fromISR( tObjeto* objeto,tMensaje mensaje, BaseType_t *pxHigherPriorityTaskWoken );
void objeto_get( tObjeto* objeto,tMensaje* mensaje );
bool objeto_get_fromISR( tObjeto* objeto,tMensaje* mensaje, BaseType_t *pxHigherPriorityTaskWoken );
void objeto_borrar( tObjeto* objeto);

#endif /* OBJETO_H_ */
