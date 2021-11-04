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

#define	MSG_MAX_SIZE				200
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
	handler->ptr_objeto2 = objeto_crear();
	handler->ptr_mensaje = pvPortMalloc(sizeof(tMensaje));
	configASSERT(handler->ptr_mensaje != NULL);
	handler->EOM = false;
	handler->SOM = false;
	handler->cantidad = 0;

	//	Reservo memoria para el memory pool
	void* Pool_puntero = pvPortMalloc(POOL_SIZE * sizeof( uint8_t ));
	configASSERT(Pool_puntero != NULL);
	//	Creo el pool de memoria
	QMPool_init(&Pool_memoria, Pool_puntero, POOL_SIZE * sizeof( uint8_t ), MSG_MAX_SIZE);  //Tamaño del segmento de memoria reservado

	uartConfig(handler->uart, handler->baudRate);
	uartCallbackSet(handler->uart, UART_RECEIVE, sf_RX_ISR, handler);

	BaseType_t res;

	res = xTaskCreate(
		tarea_recibir_paquete_de_UART,		   // Funcion de la tarea a ejecutar
		(const char *)"tarea_recibir_paquete", // Nombre de la tarea como String amigable para el usuario
		configMINIMAL_STACK_SIZE * 2,		   // Cantidad de stack de la tarea
		handler,							   // Parametros de tarea
		tskIDLE_PRIORITY + 1,				   // Prioridad de la tarea
		0									   // Puntero a la tarea creada en el sistema
	);

	configASSERT(res = pdPASS);

	res = xTaskCreate(
		tarea_enviar_paquete_por_UART,		  // Funcion de la tarea a ejecutar
		(const char *)"tarea_enviar_paquete", // Nombre de la tarea como String amigable para el usuario
		configMINIMAL_STACK_SIZE * 2,		  // Cantidad de stack de la tarea
		handler,							  // Parametros de tarea
		tskIDLE_PRIORITY + 1,				  // Prioridad de la tarea
		0									  // Puntero a la tarea creada en el sistema
	);

	configASSERT(res = pdPASS);

	handler->sem_ISR = xSemaphoreCreateBinary();
	handler->sem_bloque_liberado = xSemaphoreCreateBinary();

	configASSERT(handler->sem_ISR != NULL);
	configASSERT(handler->sem_bloque_liberado != NULL);

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
bool sf_reception_set(sf_t* handler, bool set_int)
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
 * @details	Si todavía no se recibió el SOM se descarta el byte.
 *			Si ya se recibió el SOM se guarda el paquete en el buffer.
 *			Si se vuelve a recibir el SOM se reinicia el paquete.
 *			Si el tamaño del paquete llega a MSG_MAX_SIZE se reinicia el paquete.
 * 
 * @param[in] handler       Puntero a la estructura de separación de frames.
 * @param[in] byte_recibido Byte que se recibió por la UART. 
 * 
 * @return true  Si llego el EOM. 
 * @return false Si todavía no llego el EOM o puntero es NULL.
 */
bool sf_recibir_byte(sf_t* handler, uint8_t byte_recibido)
{
	bool resp = false;
	if(handler != NULL)
	{
		if (byte_recibido == SOM_BYTE)
		{
			handler->SOM = true;
			handler->cantidad = 0;
		}
		if (handler->SOM  )
		{
			handler->buffer[handler->cantidad] = byte_recibido;
			handler->cantidad++;
			if (byte_recibido == EOM_BYTE)
			{
				handler->EOM = true;
				resp = true;
			}
			if((handler->cantidad == MSG_MAX_SIZE) && (handler->EOM == false)) // Si llegue al maximo tamaño de paquete y no recibí el EOM reinicio
			{
				handler->cantidad = 0;
				handler->SOM = false;
			}
		}
	}
return resp;
}

/**
 * @brief Valida si el CRC recibido es correcto.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 * 
 * @return true  Si el CRC es correcto.
 * @return false Si el CRC es incorrecto.
 */
bool sf_validar_crc8(sf_t* handler)
{
	uint8_t CRC_paquete,crc;
	/* convierto el CRC de ASCII a entero */
	if (sf_byte_valido(handler->buffer[handler->cantidad - 2]))
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
 * @brief Verifica que el byte sea un ASCII entre 0 y 9 o A y F. 
 * 
 * @param[in] byte Byte a verificar.
 * 
 * @return true  Si el byte es correcto.
 * @return false Si el byte es incorrecto.
 */
bool sf_byte_valido(uint8_t byte)
{
	if (( (byte >= '0')&&(byte <= '9') )||( (byte >= 'A' )&&(byte<='F') ))
		return true;
	return false;
}

/**
 * @brief Convierte un byte ASCII en entero.
 * 
 * @param[in] byte Byte a convertir.
 * 
 * @return uint8_t Byte convertido a entero.
 * 
 * @attention Requiere que el byte se encuentre previamente validado con función sf_byte_valido.
 */
uint8_t sf_atoi(uint8_t byte)
{
	if((byte >= '0')&&(byte <= '9'))
		return (byte - 48);
	return (byte - 55);			// Si no esta entre 0 y 9 se que esta entre A y F
}

/**
 * @brief Asigna un nuevo bloque de memoria del pool para recibir un mensaje.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 *  
 * @return true  Si había memoria disponible. 
 * @return false Si no hay memoria disponible.
 */
bool sf_bloque_de_memoria_nuevo(sf_t* handler)
{
	handler->buffer = (uint8_t*) QMPool_get(&Pool_memoria, 0); // Pido un bloque del pool
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
bool sf_paquete_validar(sf_t* handler)
{
	bool resp = false;
	//falta validar el ID
	if (sf_validar_crc8(handler))
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
void sf_mensaje_recibir(sf_t* handler)
{
	objeto_get(handler->ptr_objeto1, handler->ptr_mensaje);
}

/**
 * @brief El driver envía con esta función el mensaje a la aplicación avisando de que hay un nuevo paquete.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_mensaje_enviar(sf_t* handler)
{
	objeto_post(handler->ptr_objeto1, *handler->ptr_mensaje);
}

/**
 * @brief El driver se queda esperando que la aplicación le pase un nuevo dato procesado 
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_mensajeProcesado_recibir(sf_t* handler)
{
	objeto_get(handler->ptr_objeto2, handler->ptr_mensaje);
}

/**
 * @brief La aplicación le avisa por acá que procesó un dato. 
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_mensajeProcesado_enviar( sf_t* handler )
{
	objeto_post(handler->ptr_objeto2, *handler->ptr_mensaje);
}

/**
 * @brief Libera un bloque de memoria del pool de memoria
 * 
 * @param[in] ptr_mensaje Puntero al mensaje. 
 */
void sf_bloque_de_memoria_liberar(tMensaje* ptr_mensaje)
{
	QMPool_put(&Pool_memoria, ptr_mensaje-INDICE_INICIO_MENSAJE); // El inicio del bloque tiene como offset el INDICE_INICIO_MENSAJE
}

/**
 * @brief Reinicia mensaje al borrar EOM, SOM y cantidad.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_reiniciar_mensaje(sf_t* handler)
{
	handler->EOM = false;
	handler->SOM = false;
	handler->cantidad = 0;
}

/**
 * @brief Tarea de recepción de paquete completo por UART.
 * 
 * @details	Pide un bloque del pool:
 *			- si no hay bloque disponible, desactiva recepción por UART y queda a la espera de la señal de bloque liberado para volver a pedirlo.
 *			- si hay bloque disponible activa recepción por UART.
 * 			
 *			Ya con bloque de memoria disponible queda a la espera de señal de paquete listo.
 *			Cuando recibe dicha señal, valida el paquete y le envía mensaje (mensaje sin SOM, ni ID, ni EOM) por cola a la aplicacion.
 *			Reincia su ciclo. 			 
 * 
 * @param[in] pvParameters Puntero a la estructura de separación de frames. 
 */
void tarea_recibir_paquete_de_UART(void* pvParameters)
{
	sf_t* handler = (sf_t*) pvParameters;

	while(1)
	{
		do	//pido un bloque del pool, si no hay bloque disponible quedo a la espera de la señal que me avisa que se libero un bloque y vuelvo a pedir memoria
		{
			handler->buffer = (uint8_t*) QMPool_get(&Pool_memoria, 0);
			if(handler->buffer == NULL)
			{
				sf_reception_set(handler, RECEPCION_DESACTIVADA);	//Si no había memoria anulo la recepcion por UART.
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

	if (sf_recibir_byte(handler, byte_recibido))	// Proceso el byte en contexto de interrupcion, si llego EOM devuelve true, sino devuelve false
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
