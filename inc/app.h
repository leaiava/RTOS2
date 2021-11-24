/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v3.0
 *===========================================================================*/

#include "separacion_frames.h"

#define CANT_PALABRAS_MIN   1
#define CANT_PALABRAS_MAX   15
#define CANT_LETRAS_MIN     1
#define CANT_LETRAS_MAX     10

void task_app(void* pvParameters);

typedef struct 
{
    uint8_t palabras[CANT_LETRAS_MAX][CANT_PALABRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje
    uint8_t error_type;                                    ///> Variable para guardar el tipo de error en el mensaje
    tMensaje mensaje;                                      ///> Variable para recibir el mensaje
} app_t;

app_t* app_crear(void);
bool app_init(app_t* handler);
