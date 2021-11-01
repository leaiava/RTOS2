/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

/*==================[inclusiones]============================================*/
#include "objeto.h"

/*==================[definiciones y macros]==================================*/

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

//  ES INVALIDO QUE LA IMNPLEMNTACIO DEL OBJETO CONOZCA ACERCA DE LAS INSTANCIAS.

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

/*==================[funciones]====================*/

tObjeto* objeto_crear()
{
    tObjeto* rv ;

    rv = pvPortMalloc( sizeof( tObjeto ) );

    configASSERT( rv != NULL );

    rv->cola  = xQueueCreate( N_QUEUE, sizeof( tMensaje ) );

    configASSERT( rv->cola != NULL );

    return rv;
}

void objeto_post( tObjeto* objeto, tMensaje mensaje )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR( objeto->cola, &mensaje, xHigherPriorityTaskWoken );
}

void objeto_get( tObjeto* objeto,tMensaje* mensaje )
{
    xQueueReceive( objeto->cola, mensaje,portMAX_DELAY );
}

void objeto_borrar( tObjeto* objeto )
{
    /* primero se destryen los objetos "hijos"*/
    vQueueDelete( objeto->cola );

    /* al final el obj */
    vPortFree( objeto );
}

