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
#include "app_callbacks.h"
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
        /* Inicializo el OA_app*/ 
        handler_app->handler_sf = handler_sf;
        handler_app->OA_app.itIsAlive = false;
        handler_app->OA_app.itIsImmortal = true; // El OA_app no debe morir nunca.
        
        /* Cargo los punteros de los OA de procesamiento en el OA_app*/
        
        handler_app->OA_C.itIsAlive = false;
        handler_app->OA_C.itIsImmortal = false;
        
        handler_app->OA_P.itIsAlive = false;
        handler_app->OA_P.itIsImmortal = false;
        
        handler_app->OA_S.itIsAlive = false;
        handler_app->OA_S.itIsImmortal = false;
        
        // Se crea el objeto activo, con el comando correspondiente y tarea asociada.
        activeObjectOperationCreate( &handler_app->OA_app, app_OAapp, activeObjectTask , handler_sf->ptr_objeto2->cola);

        /* Cargo cola para recibir los paquetes provenientes de C2*/ 
        activeObjectQueueChange( &handler_app->OA_app , handler_sf->ptr_objeto1->cola);
        
        return true;
    }

    return false;
} 
