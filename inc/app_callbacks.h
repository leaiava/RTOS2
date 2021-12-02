/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 29/11/2021
 * Version: v1.0
 *===========================================================================*/

#ifndef __APP_CALLBACKS_H__
#define __APP_CALLBACKS_H__

#define PAQUETE     1
#define RESPUESTA   2

#include "AO.h"


void app_OAapp(void* caller_ao, void* mensaje_a_procesar );
void app_OAC(void* caller_ao, void* mensaje_a_procesar);
void app_OAP(void* caller_ao, void* mensaje_a_procesar);
void app_OAS(void* caller_ao, void* mensaje_a_procesar);

#endif