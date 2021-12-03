/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v4.0
 *===========================================================================*/

#include "app.h"
#include "AO.h"
#include "app_callbacks.h"

activeObject_t OA_app;
activeObject_t OA_C;
activeObject_t OA_P;
activeObject_t OA_S;

/**
 * @brief Asigna memoria para una estructura de app, la inicializa y crea el OA_app.
 * 
 * @param handler_app   Puntero del tipo app_t
 * @param handler_sf    Puntero del tipo sf_t, requiere que este inicializado.
 * @return true         Si salio todo bien
 * @return false        Si hubo un problema
 */
bool app_crear(app_t* handler_app , sf_t* handler_sf)
{
	if ( handler_sf != NULL )
    {
        handler_app = pvPortMalloc(sizeof(app_t));
        configASSERT(handler_app != NULL);
        
        handler_app->handler_sf = handler_sf;
        handler_app->error_type = SIN_ERROR;
        
	    OA_app.itIsAlive = false;
        OA_app.itIsImmortal = true;
        OA_app.handler_app = handler_app;
        OA_app.ptr_OA_C = &OA_C;
        OA_app.ptr_OA_P = &OA_P;
        OA_app.ptr_OA_S = &OA_S;

        OA_C.itIsAlive = false;
        OA_C.itIsImmortal = false;
        OA_C.handler_app = handler_app;

	    OA_P.itIsAlive = false;
        OA_P.itIsImmortal = false;
        OA_P.handler_app = handler_app;
        
	    OA_S.itIsAlive = false;
        OA_S.itIsImmortal = false;
        OA_S.handler_app = handler_app;

        // Se crea el objeto activo, con el comando correspondiente y tarea asociada.
        activeObjectOperationCreate( &OA_app, app_OAapp, activeObjectTask , handler_sf->ptr_objeto2->cola);

        OA_app.activeObjectQueue = handler_sf->ptr_objeto1->cola;
  
        return true;
    }

    return false;
} 
