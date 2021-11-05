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

#define	MSG_MAX_SIZE				200	// R_C2_2
#define SOM_BYTE					'('
#define EOM_BYTE					')'
#define CANT_BYTE_HEADER			8	// 1 SOM + 4 ID + 2 CRC + 1 EOM
#define INDICE_INICIO_MENSAJE		5	// El mensaje para la aplicación comienza en el 5to byte
#define INDICE_INICIO_ID			1
#define CANT_BYTE_FUERA_CRC			4
#define POOL_SIZE					1000
#define	RECEPCION_ACTIVADA			true
#define RECEPCION_DESACTIVADA		false

bool sf_reception_set(sf_t* handler, bool set_int);
bool sf_recibir_byte(sf_t* handler, uint8_t byte_recibido);
bool sf_paquete_validar(sf_t* handler);
bool sf_validar_crc8(sf_t* handler);
bool sf_byte_valido(uint8_t byte);
bool sf_bloque_de_memoria_nuevo(sf_t* handler);
uint8_t sf_atoi(uint8_t byte);
void sf_bloque_de_memoria_liberar(tMensaje* ptr_mensaje);
void sf_reiniciar_mensaje(sf_t* handler);
void sf_mensaje_enviar( sf_t* handler );
void sf_mensajeProcesado_recibir( sf_t* handler );
void tarea_recibir_paquete_de_UART(void* pvParameters);
void tarea_enviar_paquete_por_UART(void* pvParameters);
void sf_RX_ISR(void* parametro);
void sf_TX_ISR(void* parametro);

QMPool Pool_memoria; // Memory pool (contienen la informacion que necesita la biblioteca qmpool.h)

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
 * param[in]		handler	puntero a la estructura de separacion de frames
 * param[in]		uart	que UART del LPC4337 vamos a usar
 * param[in]		baudRate	El baudRate que se va a usar en la comunicación serie
 * param[in]		callbackFunc	Puntero a la función de callback, cada vez que llega un dato a la
 * 									UART se dispara esta función.
 * @return 			true cuando fue todo correcto
 * @return 			false cuando los punteros no cumplen con tener sector de memoria
 */

bool sf_init(sf_t* handler,uartMap_t uart, uint32_t baudRate,callBackFuncPtr_t callbackFunc )
{
	bool resp=false;
	if (!((callbackFunc == NULL) || (handler == NULL))){
		handler->uart = uart;
		handler->baudRate = baudRate;
		handler->buffer = pvPortMalloc(sizeof(uint8_t)*MSG_MAX_SIZE);
		configASSERT( handler->buffer != NULL );
		uartConfig(handler->uart , handler->baudRate);
		uartCallbackSet(handler->uart , UART_RECEIVE, callbackFunc, NULL);
		resp = true;
	}
	return resp;
}

/**
 * @brief	Activa o desactiva la recepcion  de frames segun la bandera de interrupción que maneja el puerto serie
 * @return 			true cuando fue todo correcto
 * @return 			false cuando los punteros no cumplen con tener sector de memoria
 */

bool sf_reception_set(sf_t* handler , bool set_int)
{
	bool resp = false;
	if(handler != NULL){
		uartInterrupt(handler->uart, set_int);
		resp = true;
	}
	return(resp);
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
		if (byte_recibido == SOM_BYTE)	// R_C2_3
		{
			handler->SOM = true;		// R_C2_4

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
	if (sf_byte_valido(handler->buffer[handler->cantidad - 3]))
		CRC_paquete += (sf_atoi(handler->buffer[handler->cantidad - 3])) << 4;
	else
		return false;	// Si el caracter de CRC no es válido retorno false

	/* calculo el CRC del paquete*/
	crc = crc8_calc(0, handler->buffer + INDICE_INICIO_ID, handler->cantidad - CANT_BYTE_FUERA_CRC);

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
	sf_t* handler = (sf_t*) pvParameters;

	while(1)
	{
		do	// R_C2_8 pido un bloque del pool, si no hay bloque disponible quedo a la espera de la señal que me avisa que se libero un bloque y vuelvo a pedir memoria
		{
			handler->buffer = (uint8_t*) QMPool_get(&Pool_memoria, 0);
			if(handler->buffer == NULL)
			{
				sf_reception_set(handler, RECEPCION_DESACTIVADA);	// R_C2_9 Si no había memoria anulo la recepcion por UART.
				xSemaphoreTake(handler->sem_bloque_liberado,portMAX_DELAY); // Quedo esperando a que se libere memoria.
			}
			else
			{
				sf_reception_set(handler, RECEPCION_ACTIVADA ); // habilito la recepcion por UART.
			}
		}
		while (handler->buffer == NULL);

		do
		{
			xSemaphoreTake(handler->sem_ISR,portMAX_DELAY);	//Espero que la ISR me indique que tiene un paquete listo
		}
		while(!(sf_paquete_validar(handler)));

		handler->ptr_mensaje->datos = handler->buffer + INDICE_INICIO_MENSAJE; //Cargo puntero con inicio de mensaje para la aplicación
		handler->ptr_mensaje->cantidad = handler->cantidad - CANT_BYTE_HEADER;
		sf_mensaje_enviar(handler);
		sf_reiniciar_mensaje(handler);
	}
}

/**
 * @brief Recibe el mensaje procesado, lo manda por la UART y libera la memoria.
 * 
 * @param[in] pvParameters Puntero a la estructura de separación de frames. 
 */
void tarea_enviar_paquete_por_UART(void* pvParameters)
{
	sf_t* handler = (sf_t*) pvParameters;
	while(1)
	{
	sf_mensajeProcesado_recibir(handler);
	//ARMAR NUEVO PAQUETE AQUÍ
	//ENVIAR PORT UART PAQUETE PROCESADO
	sf_bloque_de_memoria_liberar(handler->ptr_mensaje);
	xSemaphoreGive(handler->sem_bloque_liberado);
	}
}

/**
 * @brief ISR de recepción por UART.
 * 
 * @param[in] parametro Puntero a la estructura de separación de frames. 
 * 
 */
void sf_RX_ISR( void *parametro )
{
	sf_t* handler = (sf_t*)parametro;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	uint8_t byte_recibido = uartRxRead( handler->uart ); // Leo byte de la UART con sAPI

	if (sf_recibir_byte(handler, byte_recibido))	// R_C2_5 Proceso el byte en contexto de interrupcion, si llego EOM devuelve true, sino devuelve false
	{
		xSemaphoreGiveFromISR(handler->sem_ISR, &xHigherPriorityTaskWoken ); // Le aviso a la tarea encargada de recibir datos que llego un paquete.
	}
}

/**
 * @brief ISR de transmisión por UART.
 * 
 * @param[in] parametro Puntero a la estructura de separación de frames. 
 */
void sf_TX_ISR( void *parametro )
{
	sf_t* handler = (sf_t*) parametro;
}
