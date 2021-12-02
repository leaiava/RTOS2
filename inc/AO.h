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
#include "task.h"
#include "semphr.h"
#include "sapi.h"
#include "app.h"

#define N_QUEUE_AO 		10

typedef void ( *callBackActObj_t )( void* caller_ao, void* data );

typedef struct
{
    char* string;
    int size;
    QueueHandle_t colaCapa3;
} activeObjectResponse_t;

typedef struct
{
    TaskFunction_t 		taskName;
    QueueHandle_t 		activeObjectQueue;
    QueueHandle_t 		responseQueue;
    callBackActObj_t 	callbackFunc;
    app_t*              handler_app;
    void*               ptr_OA_C;
    void*               ptr_OA_P;
    void*               ptr_OA_S;
    bool 				itIsAlive;
    bool                itIsImmortal;
    
} activeObject_t;

bool_t activeObjectCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO );

void activeObjectTask( void* pvParameters );

void activeObjectEnqueue( activeObject_t* ao, void* value );
bool_t activeObjectOperationCreate( activeObject_t* ao, callBackActObj_t callback, TaskFunction_t taskForAO, QueueHandle_t response_queue );

#endif /* AO_H__ */
