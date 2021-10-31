/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

#include "separacion_frames.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include <string.h>

#define	MSG_MAX_SIZE	200
#define SOM_BYTE	'('
#define EOM_BYTE	')'
#define CANT_BYTE_HEADER	8	// 1 SOM + 4 ID + 2 CRC + 1 EOM
#define INDICE_INICIO_MENSAJE 4	// El mensaje comienza en el 5 byte, indice 4.

/**
 * @brief devuelve un puntero a una estructura de separcion de frames
 */
sf_t* sf_crear(void)
{
	sf_t* me = pvPortMalloc(sizeof(sf_t));
	configASSERT( me != NULL );
	return me;
}

/**
 * @brief	inicializa la estructura de separacion de frame
 * param[in,out]	handler	puntero a la estructura de separacion de frames
 * param[in]		uart	que UART del LPC4337 vamos a usar
 * param[in]		baudRate	El baudRate que se va a usar en la comunicación serie
 * param[in]		callbackFunc	Puntero a la función de callback, cada vez que llega un dato a la
 * 									UART se dispara esta función.
 */
void sf_init(sf_t* handler,uartMap_t uart, uint32_t baudRate,callBackFuncPtr_t callbackFunc )
{
	if (callbackFunc == NULL)
		return;

	handler->uart = uart;
	handler->baudRate = baudRate;
	handler->buffer = pvPortMalloc(sizeof(uint8_t)*MSG_MAX_SIZE);
	configASSERT( handler->buffer != NULL );

	uartConfig(handler->uart , handler->baudRate);

	uartCallbackSet(handler->uart , UART_RECEIVE, callbackFunc, NULL);

	return;
}

/**
 * @brief	Activa la separacion de frames activando la interrupción que maneja el puerto serie
 */
void sf_activar(sf_t* handler)
{
	uartInterrupt(handler->uart, true);
}

/**
 * "brief	Desactiva la separacion de frames activando la interrupción que maneja el puerto serie
 */
void sf_desactivar(sf_t* handler)
{
	uartInterrupt(handler->uart, false);
}

/**
 * @brief	Recibe un byte y lo coloca en el buffer
 * param[in,out]	handler	puntero a la estructura de separacion de frames
 * param[in]		byte_recibido	Byte que se recibió por la UART.
 *
 * @details		Si todavía no se recibió el SOM se descarta el byte
 * 				Si ya se recibió el SOM se guarda el paquete en el buffer
 * 				Si se vuelve a recibir el SOM se reinicia el paquete
 * 				Si el tamaño del paquete llega a MSG_MAX_SIZE se reinicia el paquete
 */

void sf_recibir_byte(sf_t* handler, uint8_t byte_recibido)
{
	if(handler == NULL)
		return;

	if (handler->SOM  )			// Accion si ya había llegado el SOM
	{
		if(byte_recibido == SOM_BYTE)								// R_C2_4
			handler->cantidad = 0;

		handler->buffer[handler->cantidad] = byte_recibido;
		if (handler->buffer[handler->cantidad] == EOM_BYTE)	// verifico si el byte recibido es el EOM
			handler->EOM = true;
		handler->cantidad++;
	}
	else						// Accion si todavía no llego el SOM
	if (byte_recibido == SOM_BYTE) //verifico que el byte recibido sea el SOM
	{
		handler->buffer[handler->cantidad] = byte_recibido;
		handler->cantidad++;
		handler->SOM = true;
	}

	if((handler->cantidad == MSG_MAX_SIZE) && (handler->EOM == false)) //Si llegue al maximo tamaño de paquete y no recibí el EOM reinico
		{
		handler->cantidad = 0;
		handler->SOM = false;
		}

	return;
}

/**
 * @brief	API para consultar si hay un mensaje completo
 * param[in,out]	handler	puntero a la estructura de separacion de frames
 *
 * return	true	si existe SOM y EOM
 * 			false	Si no existe SOM y EOM
 */
bool sf_mensaje_esta_completo(sf_t* handler)
{
	if (handler->SOM && handler->EOM)
		return true;
	return false;
}

/**
 * @brief	Separa el mensaje del paquete
 * param[in,out]	handler	puntero a la estructura de separacion de frames
 *
 * details	De acuerdo al tamaño del paquete pide memoria dinámica para guardar el mensaje
 * return	ptr_mensaje	: Devuelve el puntero al mensaje recibido, si no había memoria disponible
 * 						  devolverla NULL.
 */
ptr_mensaje sf_separar_mensaje(sf_t* handler)
{
	ptr_mensaje puntero = pvPortMalloc(handler->cantidad - CANT_BYTE_HEADER);
	if (puntero == NULL)
		return puntero;
	memcpy(puntero, handler->buffer+INDICE_INICIO_MENSAJE, (handler->cantidad - CANT_BYTE_HEADER));
	return puntero;
}

/**
 * @brief	Valida si el CRC recibido es correcto
 * param[in,out]	handler	puntero a la estructura de separacion de frames
 *
 * return	true	Si el CRC es correto
 * 			false	Si el CRC es incorrecto
 */
bool sf_validar_crc8(sf_t* handler)
{
	uint8_t CRC_paquete,crc;
	/* convierto el CRC de ASCII a entero */
	if ( sf_byte_valido(handler->buffer[handler->cantidad - 2]) )
		CRC_paquete = sf_atoi(handler->buffer[handler->cantidad - 2]);
	else
		return false;	// Si el caracter de CRC no es válido retorno false
	if ( sf_byte_valido(handler->buffer[handler->cantidad - 3]) )
		CRC_paquete += sf_atoi(handler->buffer[handler->cantidad - 3]);
	else
		return false;	// Si el caracter de CRC no es válido retorno false

	/* calculo el CRC del paquete*/
	crc = crc8_calc( 0 , handler->buffer , handler->cantidad );

	if (crc == CRC_paquete)
		return true;	// Si el CRC es correcto devuelvo true
	return false;		// Si el CRC es incorrecto devuelvo false
}

/**
 * @brief verifica que el byte sea un ASCII entre 0 y 9 o A y F
 * param[in]	byte Byte a verificar.
 *
 * return	true	Si el byte es correcto
 * 			false	Si el byte es incorrecto
 */

static bool sf_byte_valido(uint8_t byte)
{
	if ( ( (byte >= '0')&&(byte <= '9') )||( (byte >= 'A' )&&(byte<='F') ) )
		return true;
	return false;
}

/**
 * @brief	convierte un byte ascii en entero
 * param[in]	byte Byte a verificar.
 *
 * @attention	Requiere que el byte sea validado con sf_byte_valido
 */
static uint8_t sf_atoi(uint8_t byte)
{
	if( (byte >= '0')&&(byte <= '9') )
		return (byte - 48);
	return (byte - 55);			//Si no esta entre 0 y 9 se que esta entre A y F
}
