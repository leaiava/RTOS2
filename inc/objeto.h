/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
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
#define objeto_post_fromISR(x,y,z)	xQueueSendFromISR(x,y,z)
#define objeto_get_fromISR(x,y,z)	xQueueReceiveFromISR(x,y,z)

typedef struct
{
	uint8_t* ptr_datos;
	uint32_t cantidad;
}tMensaje;

typedef struct
{
    QueueHandle_t cola;
} tObjeto;

tObjeto* objeto_crear();
void objeto_post( tObjeto* objeto,tMensaje mensaje );
void objeto_get( tObjeto* objeto,tMensaje* mensaje );
void objeto_borrar( tObjeto* objeto);

#endif /* OBJETO_H_ */
