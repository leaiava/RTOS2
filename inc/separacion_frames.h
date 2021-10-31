/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

#ifndef separacion_frames_H_
#define separacion_frames_H_

#include "sapi.h"
#include "crc8.h"

typedef uint32_t* ptr_mensaje;

typedef struct
{
	uartMap_t	uart;		///< Nombre de la UART del LPC4337 a utilizar
	uint32_t 	baudRate;	///< BaudRate seleccionado para la comunicacion serie
	uint32_t*	buffer;		///< buffer para la recepción de bytes
	uint32_t	cantidad;	///< cantidad de bytes recibidos
	bool		SOM;		///< flag para indicar si llego el SOM
	bool		EOM;		///< flag para indicar si llego el EOM
}sf_t;

sf_t* sf_crear(void);
void sf_init(sf_t* handler,uartMap_t uart, uint32_t baudRate,callBackFuncPtr_t callbackFunc );
void sf_activar(sf_t* handler);
void sf_desactivar(sf_t* handler);
void sf_recibir_byte(sf_t* handler, uint8_t byte_recibido);
bool sf_mensaje_esta_completo(sf_t* handler);
ptr_mensaje sf_separar_mensaje(sf_t* handler);
bool sf_validar_crc8(sf_t* handler);
static bool sf_byte_valido(uint8_t byte);
static uint8_t sf_atoi(uint8_t byte);

#endif /* separacion_frames_H_ */
