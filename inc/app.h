/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v3.0
 *===========================================================================*/
#ifndef APP_H_
#define APP_H_

#include "separacion_frames.h"

#define CANT_PALABRAS_MIN       1   // R_C3_2
#define CANT_PALABRAS_MAX       15  // R_C3_1
#define CANT_LETRAS_MIN         1   // R_C3_4
#define CANT_LETRAS_MAX         10  // R_C3_3

#define SIN_ERROR               -1
#define ERROR_INVALID_DATA      0
#define ERROR_INVALID_OPCODE    1
#define ERROR_SYSTEM            2

#define CARACTER_INICIAL        0
#define PALABRA_INICIAL         0

#define INDICE_CAMPO_DATOS      1
#define INDICE_CAMPO_C          0

#define A_MINUSCULA             32  // 32 es la diferencia entre un caracter en mayúscula y uno en minúscula.
#define A_MAYUSCULA             -32
void task_app(void* pvParameters);

typedef struct 
{
	uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje
    uint8_t error_type;                                    ///> Variable para guardar el tipo de error en el mensaje
    tMensaje mensaje;                                      ///> Variable para recibir el mensaje
    sf_t* handler_sf;                                      ///> Handler para la capa de separación de frame
} app_t;

bool app_crear(app_t* handler_app , sf_t* handler_sf);

#endif /* APP_H_ */ 