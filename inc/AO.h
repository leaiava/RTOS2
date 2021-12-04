/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v1.0
 *===========================================================================*/

#ifndef AO_H__
#define AO_H__

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "sapi.h"
#include "separacion_frames.h"

#define N_QUEUE_AO 		10

typedef void ( *callBackActObj_t )( void* caller_ao, void* data );

typedef struct
{
    TaskFunction_t 		taskName;
    QueueHandle_t 		activeObjectQueue;
    QueueHandle_t 		responseQueue;
    callBackActObj_t 	callbackFunc;
    sf_t*               ptr_sf;
    void*               ptr_OA_C;
    void*               ptr_OA_P;
    void*               ptr_OA_S;
    bool 				itIsAlive;
    bool                itIsImmortal;

} activeObject_t;

bool activeObjectCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO );

void activeObjectTask( void* pvParameters );

void activeObjectEnqueue( activeObject_t* ao, void* value );
void activeObjectEnqueueResponse( activeObject_t* ao, void* value );
bool activeObjectOperationCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO, QueueHandle_t response_queue );
void activeObjectQueueChange( activeObject_t* ao, QueueHandle_t activeObjectNewQueue );

#endif /* AO_H__ */
