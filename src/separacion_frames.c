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
#include "task.h"
#include <string.h>
#include "sepa_frame_def.h"

static bool sf_reception_set(sf_t* handler, bool set_int);
static bool sf_recibir_byte(sf_t* handler, uint8_t byte_recibido);
static bool sf_paquete_validar(sf_t* handler);
static bool sf_validar_id(sf_t* handler);
static bool sf_validar_crc8(sf_t* handler);
static bool sf_byte_valido(uint8_t byte);
static bool sf_bloque_de_memoria_nuevo(sf_t* handler);
static uint8_t sf_decodificar_ascii(uint8_t byte);
static void sf_bloque_de_memoria_liberar(sf_t* handler);
static void sf_reiniciar_mensaje(sf_t* handler);
static void sf_mensaje_enviar( sf_t* handler );
static void sf_mensaje_procesado_recibir( sf_t* handler );
static void tarea_recibir_paquete_uart(void* pvParameters);
static void tarea_enviar_paquete_uart(void* pvParameters);
static void sf_rx_isr(void* parametro);
static void sf_tx_isr(void* parametro);

/**
 * @brief Asigna memoria para una estructura de separcion de frames y devuelve puntero a ella.
 * 
 * @return sf_t* Puntero a una estructura de separcion de frames.
 */
sf_t* sf_crear(void)
{
	sf_t* me = pvPortMalloc(sizeof(sf_t));
	configASSERT(me != NULL);
	return me;
}

/**
 * @brief Inicializa la estructura de separación de frame.
 * 
 * @param[in] handler	Puntero a la estructura de separación de frames.
 * @param[in] uartQue	Que UART del LPC4337 vamos a usar.
 * @param[in] baudRate	El baudRate que se va a usar en la comunicación serie.
 * 
 * @return true  Cuando fue todo correcto.
 * @return false Cuando los punteros no cumplen con tener sector de memoria.
 */
bool sf_init(sf_t* handler, uartMap_t uart, uint32_t baudRate)
{
	if (handler == NULL)
		return false;

	handler->uart = uart;
	handler->baudRate = baudRate;
	handler->ptr_objeto1 = objeto_crear();
	handler->ptr_mensaje = pvPortMalloc(sizeof(tMensaje));
	configASSERT(handler->ptr_mensaje != NULL);
	handler->EOM = false;
	handler->SOM = false;
	handler->cantidad = 0;

	//	Reservo memoria para el memory pool
	handler->prt_pool = pvPortMalloc(POOL_SIZE * sizeof( uint8_t ));
	configASSERT(handler->prt_pool != NULL);
	//	Creo el pool de memoria
	QMPool_init(&(handler->pool_memoria), handler->prt_pool, POOL_SIZE * sizeof( uint8_t ), MSG_MAX_SIZE);  //Tamaño del segmento de memoria reservado

	uartConfig(handler->uart, handler->baudRate);
	uartCallbackSet(handler->uart, UART_RECEIVE, sf_rx_isr, handler);
	return true;
}

/**
 * @brief Activa o desactiva la recepcion de frames segun la bandera de interrupción que maneja el puerto serie.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames. 
 * @param[in] set_int Activar o Desactivar.
 * 
 * @return true  Cuando fue todo correcto. 
 * @return false Cuando los punteros no cumplen con tener sector de memoria.
 */
static bool sf_reception_set(sf_t* handler, bool set_int)
{
	bool resp = false;
	if(handler != NULL){
		uartInterrupt(handler->uart, set_int);
		resp = true;
	}
	return(resp);
}

/**
 * @brief Recibe un byte y lo coloca en el buffer.
 * 
 * @details Si todavía no se recibió el SOM se descarta el byte.
 *          Si ya se recibió el SOM se guarda el paquete en el buffer.
 *          Si se vuelve a recibir el SOM se reinicia el paquete.
 *          Si el tamaño del paquete llega a MSG_MAX_SIZE se reinicia el paquete.
 * 
 * @param[in] handler       Puntero a la estructura de separación de frames.
 * @param[in] byte_recibido Byte que se recibió por la UART. 
 * 
 * @return true  Si llegó el EOM. 
 * @return false Si todavía no llego el EOM o puntero es NULL.
 */
static bool sf_recibir_byte(sf_t* handler, uint8_t byte_recibido)
{
	bool resp = false;
	if(handler != NULL)
	{
		if (byte_recibido == SOM_BYTE)	// R_C2_3
		{
			handler->SOM = true;		// R_C2_4
			handler->cantidad = 0;
		}
		if (handler->SOM  )
		{
			handler->buffer[handler->cantidad] = byte_recibido;	// R_C2_6
			handler->cantidad++;
			if (byte_recibido == EOM_BYTE)	// R_C2_3
			{
				handler->EOM = true;
				resp = true;
			}
			if((handler->cantidad == MSG_MAX_SIZE) && (handler->EOM == false)) // R_C2_7 Si llegue al maximo tamaño de paquete y no recibí el EOM reinicio
			{
				handler->cantidad = 0;
				handler->SOM = false;
			}
		}
	}
return resp;
}

/**
 * @brief Valida si el ID recibido es correcto.
 * 
 * @return true  Si el ID es correcto.
 * @return false Si el ID es incorrecto.
 */
static bool sf_validar_id(sf_t* handler)
{
	for(uint8_t i = INDICE_INICIO_ID ; i < (INDICE_INICIO_ID + LEN_ID) ; i++)
	{
		if(!sf_byte_valido(handler->buffer[i]))
			return false;
	}
	return true;
}

/**
 * @brief Valida si el CRC recibido es correcto.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 * 
 * @return true  Si el CRC es correcto.
 * @return false Si el CRC es incorrecto.
 */
static bool sf_validar_crc8(sf_t* handler)
{
	uint8_t CRC_paquete,crc;
	/* convierto el CRC de ASCII a entero */
	if (sf_byte_valido(handler->buffer[handler->cantidad - POS_CRC_H]))
		CRC_paquete = sf_decodificar_ascii(handler->buffer[handler->cantidad - POS_CRC_H]);
	else
		return false;	// Si el caracter de CRC no es válido retorno false
	if (sf_byte_valido(handler->buffer[handler->cantidad - POS_CRC_L]))
		CRC_paquete += (sf_decodificar_ascii(handler->buffer[handler->cantidad - POS_CRC_L])) << S_LEFT_4b;
	else
		return false;	// Si el caracter de CRC no es válido retorno false

	/* calculo el CRC del paquete*/
	crc = crc8_calc(0, handler->buffer + INDICE_INICIO_ID, handler->cantidad - CANT_BYTE_FUERA_CRC);

	if (crc == CRC_paquete)
		return true;	// Si el CRC es correcto devuelvo true
	return false;		// Si el CRC es incorrecto devuelvo false
}

/**
 * @brief Verifica que el byte sea un ASCII entre 0 y 9 o A y F. 
 * 
 * @param[in] byte Byte a verificar.
 * 
 * @return true  Si el byte es correcto.
 * @return false Si el byte es incorrecto.
 */
static bool sf_byte_valido(uint8_t byte)
{
	if (( (byte >= ASCII_0)&&(byte <= ASCII_9) )||( (byte >= ASCII_A )&&(byte<= ASCII_F) ))
		return true;
	return false;
}

/**
 * @brief Decodifica un byte en ASCII.
 * 
 * @param[in] byte Byte a decodificar.
 * 
 * @return uint8_t Byte decodificado.
 * 
 * @attention Requiere que el byte se encuentre previamente validado con la función sf_byte_valido.
 */
static uint8_t sf_decodificar_ascii(uint8_t byte)
{
	if((byte >= ASCII_0)&&(byte <= ASCII_9))
		return (byte - ASCII_0);
	return (byte - ASCII_TO_NUM);			// Si no está entre 0 y 9, está entre A y F
}

/**
 * @brief Asigna un nuevo bloque de memoria del pool para recibir un mensaje.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 *  
 * @return true  Si había memoria disponible. 
 * @return false Si no hay memoria disponible.
 */
static bool sf_bloque_de_memoria_nuevo(sf_t* handler)
{
	handler->buffer = (uint8_t*) QMPool_get(&(handler->pool_memoria), 0); // Pido un bloque del pool
	if(handler->buffer == NULL)
		return false;
	return true;
}

/**
 * @brief Valida el campo ID y CRC del paquete.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 * 
 * @return true  ID y CRC del paquete válidos.
 * @return false ID y/o CRC del paquete inválidos.
 */
static bool sf_paquete_validar(sf_t* handler)
{
	bool resp = false;
	
	if (sf_validar_crc8(handler) && sf_validar_id(handler))
		resp = true;
	else
		sf_reiniciar_mensaje(handler);
	return resp;
}

/**
 * @brief Le sirve a la aplicación para esperar el mensaje de nuevo paquete.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 *
 */
bool sf_mensaje_recibir(sf_t* handler)
{
	//  Pido un bloque de memoria nuevo, en caso de error termino.
	if (!sf_bloque_de_memoria_nuevo(handler))
		return false;								// R_C2_9

	sf_reception_set(handler, RECEPCION_ACTIVADA ); // habilito la recepcion por UART.

	objeto_get(handler->ptr_objeto1, handler->ptr_mensaje);
	return true;
}

/**
 * @brief El driver envía con esta función el mensaje a la aplicación avisando de que hay un nuevo paquete.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
static void sf_mensaje_enviar(sf_t* handler)
{
	objeto_post(handler->ptr_objeto1, *handler->ptr_mensaje);
}

/**
 * @brief La aplicación le avisa por acá que procesó un dato. 
 * 
 * @details Dispara la interuupcion tx_isr, mientras haya espacio en el buffer de transmisión se ejecuta la tx_isr
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_mensaje_procesado_enviar( sf_t* handler )
{
	//Calcular nuevo CRC aqui
	uartCallbackSet(handler->uart, UART_TRANSMITER_FREE, sf_tx_isr, handler);
	uartSetPendingInterrupt(handler->uart);
}

/**
 * @brief Libera un bloque de memoria del pool de memoria
 * 
 * @param[in] ptr_mensaje Puntero al mensaje. 
 */
void sf_bloque_de_memoria_liberar(sf_t* handler)
{
	QMPool_put(&(handler->pool_memoria), handler->ptr_mensaje-INDICE_INICIO_MENSAJE); // El inicio del bloque tiene como offset el INDICE_INICIO_MENSAJE
}

/**
 * @brief Reinicia mensaje al borrar EOM, SOM y cantidad.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
static void sf_reiniciar_mensaje(sf_t* handler)
{
	handler->EOM = false;
	handler->SOM = false;
	handler->cantidad = 0;
}

/**
 * @brief ISR de recepción por UART.
 * 
 * @param[in] parametro Puntero a la estructura de separación de frames. 
 * 
 */
static void sf_rx_isr( void *parametro )
{
	sf_t* handler = (sf_t*)parametro;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	uint8_t byte_recibido = uartRxRead( handler->uart ); // Leo byte de la UART con sAPI

	if (sf_recibir_byte(handler, byte_recibido))	// R_C2_5 Proceso el byte en contexto de interrupcion, si llego EOM devuelve true, sino devuelve false
	{
		if (sf_paquete_validar(handler))
		{
			handler->ptr_mensaje->datos = handler->buffer + INDICE_INICIO_MENSAJE; // Cargo puntero con inicio de mensaje para la aplicación
			handler->ptr_mensaje->cantidad = handler->cantidad - LEN_HEADER;
			sf_mensaje_enviar(handler);
			sf_reiniciar_mensaje(handler);

		}
	}
}

/**
 * @brief ISR de transmisión por UART.
 * 
 * @param[in] parametro Puntero a la estructura de separación de frames. 
 */
static void sf_tx_isr( void *parametro )
{
	sf_t* handler = (sf_t*) parametro;
	static uint32_t indice_byte_enviado = 0;
	
	
	uartTxWrite(handler->uart, *(handler->ptr_mensaje->datos - INDICE_INICIO_MENSAJE + indice_byte_enviado) );
	indice_byte_enviado++;
	if ( indice_byte_enviado == (handler->ptr_mensaje->cantidad + LEN_HEADER))
	{
		indice_byte_enviado = 0;
		/*
		sf_bloque_de_memoria_liberar(handler);
		if(handler->sin_memoria == true)
		{
			sf_bloque_de_memoria_nuevo(handler);
			handler->sin_memoria = false;
		}
*/
		uartCallbackClr(handler->uart, UART_TRANSMITER_FREE); //Elimino el callback para eliminar la tx_isr
	}
	
	
}
