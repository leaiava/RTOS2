/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v1.0
 *===========================================================================*/

#include "AO.h"

bool activeObjectCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO )
{
    // Una variable local para saber si hemos creado correctamente los objetos.
    BaseType_t retValue = pdFALSE;

    // Creamos la cola asociada a este objeto activo.
    ao->activeObjectQueue = xQueueCreate( N_QUEUE_AO, sizeof( void* ) );

    // Asignamos la tarea al objeto activo.
    ao->taskName = taskForAO;

    // Si la cola se creó sin inconvenientes.
    if( ao->activeObjectQueue != NULL )
    {
        // Asignamos el callback al objeto activo.
        ao->callbackFunc = callback;

        // Creamos la tarea asociada al objeto activo. A la tarea se le pasar� el objeto activo como par�metro.
        retValue = xTaskCreate( ao->taskName, ( const char * )"Task For AO", configMINIMAL_STACK_SIZE*2, ao, tskIDLE_PRIORITY+2, NULL );
    }

    // Chequeamos si la tarea se cre� correctamente o no.
    if( retValue == pdPASS )
    {
        // Cargamos en la variable de estado del objeto activo el valor "true" para indicar que se ha creado.
        ao->itIsAlive = TRUE;

        // Devolvemos "true" para saber que el objeto activo se instanci� correctamente.
        return( TRUE );
    }
    else
    {
        // Caso contrario, devolvemos "false".
        return( FALSE );
    }
}

void activeObjectTask( void* pvParameters )
{
    // Una variable para evaluar la lectura de la cola.
    BaseType_t retQueueVal;

    // Una variable local para almacenar el dato desde la cola.
    void*  auxValue;

    // Obtenemos el puntero al objeto activo.
    activeObject_t* actObj = ( activeObject_t* ) pvParameters;

    // Cuando hay un evento, lo procesamos.
    while( TRUE )
    {
        // Verifico si hay elementos para procesar en la cola. Si los hay, los proceso. Si soy inmortal entro siempre y si no hay mensajes espero.
        if( (uxQueueMessagesWaiting( actObj->activeObjectQueue )) || actObj->itIsImmortal )
        {
            // Hago una lectura de la cola.
            retQueueVal = xQueueReceive( actObj->activeObjectQueue, &auxValue, portMAX_DELAY );

            // Si la lectura fue exitosa, proceso el dato.
            if( retQueueVal )
            {
                // Llamamos al callback correspondiente en base al comando que se le pas�.
                /* TODO:INFO a la funcion llamante le mando el ao que la llamo coo referenca porq
                   es necesario */
                ( actObj->callbackFunc )( actObj,  &auxValue );
            }
        }

        // Caso contrario, la cola est� vac�a, lo que significa que debo eliminar la tarea.
        else
        {
            // Cambiamos el estado de la variable de estado, para indicar que el objeto activo no existe más.
            actObj->itIsAlive = FALSE;

            // Borramos la cola del objeto activo.
            vQueueDelete( actObj->activeObjectQueue );

            // Y finalmente tenemos que eliminar la tarea asociada (suicidio).
            vTaskDelete( NULL );

        }
    }
}

void activeObjectEnqueue( activeObject_t* ao, void* value )
{
    // Y lo enviamos a la cola.
    xQueueSend( ao->activeObjectQueue, value, 0 );
}

void activeObjectOperationCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO, QueueHandle_t response_queue )
{
    /* cargo miembro que no estaba */
    ao->responseQueue= response_queue;
    /* creo oa padre */
    activeObjectCreate( ao, callback, taskForAO );
}
