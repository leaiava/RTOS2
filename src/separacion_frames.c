/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 12/11/2021
 * Version: v2.0
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
static void sf_rx_isr(void* parametro);
static void sf_tx_isr(void* parametro);
static void timer_callback(TimerHandle_t xTimer);

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
	handler->EOM = false;
	handler->SOM = false;
	handler->out_of_memory = false;
	handler->cantidad = 0;

	//	Reservo memoria para el memory pool
	handler->prt_pool = pvPortMalloc(POOL_SIZE * sizeof( uint8_t ));
	configASSERT(handler->prt_pool != NULL);
	//	Creo el pool de memoria
	QMPool_init(&(handler->pool_memoria), handler->prt_pool, POOL_SIZE * sizeof( uint8_t ), MSG_MAX_SIZE);  //Tamaño del segmento de memoria reservado
	//Pido un bloque de memoria
	configASSERT(sf_bloque_de_memoria_nuevo(handler) == true);

	uartConfig(handler->uart, handler->baudRate);
	uartCallbackSet(handler->uart, UART_RECEIVE, sf_rx_isr, handler);

    handler->periodo_timer = TIMEOUT;

    handler->timer = xTimerCreate(
                    "Timer",
                    handler->periodo_timer, //Timeout
                    pdFALSE,                //One-shot
                    handler,                //Estructura de datos que llega al callback
                    timer_callback
    );

    configASSERT(handler->timer != NULL);
    // Habilito recepcion por UART.
    sf_reception_set(handler, RECEPCION_ACTIVADA);
    gpioInit( GPIO0, GPIO_OUTPUT ); // Para debug timer
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
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(handler != NULL)
	{
		if (byte_recibido == SOM_BYTE)	// R_C2_3
		{
			handler->SOM = true;		// R_C2_4
			handler->cantidad = 0;
		}
		if (handler->SOM  )
		{
			xTimerStartFromISR(handler->timer, &xHigherPriorityTaskWoken);         // R_C2_18
			gpioWrite(GPIO0, ON);                             // Para debug timer

			handler->buffer[handler->cantidad] = byte_recibido;	// R_C2_6
			handler->cantidad++;
			if (byte_recibido == EOM_BYTE)	// R_C2_3
			{
				xTimerStopFromISR(handler->timer, &xHigherPriorityTaskWoken);    // Si se recibe EOM detiene el timer
				handler->EOM = true;
				resp = true;
			}
			if((handler->cantidad == MSG_MAX_SIZE) && (handler->EOM == false)) // R_C2_7 Si llegue al maximo tamaño de paquete y no recibí el EOM reinicio
			{
				xTimerStopFromISR(handler->timer, &xHigherPriorityTaskWoken);
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
		CRC_paquete += (sf_decodificar_ascii(handler->buffer[handler->cantidad - POS_CRC_L])) << SHIFT_4b;
	else
		return false;	// Si el caracter de CRC no es válido retorno false

	/* calculo el CRC del paquete*/
	crc = crc8_calc(0, handler->buffer + INDICE_INICIO_ID, handler->cantidad - CANT_BYTE_FUERA_CRC); // R_C2_20

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
	
	if (sf_validar_crc8(handler) && sf_validar_id(handler)) //R_C2_11
		resp = true;

	return resp;
}

/**
 * @brief Le sirve a la aplicación para esperar el mensaje de nuevo paquete.
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 *
 */
bool sf_mensaje_recibir(sf_t* handler, tMensaje* ptr_mensaje)
{
	objeto_get(handler->ptr_objeto1, ptr_mensaje);
		return true;
}

/**
 * @brief La aplicación le avisa por acá que procesó un dato. 
 * 
 * @details Dispara la interrupción tx_isr, mientras haya espacio en el buffer de transmisión se ejecuta la tx_isr
 * 
 * @param[in] handler Puntero a la estructura de separación de frames.
 */
void sf_mensaje_procesado_enviar( sf_t* handler, tMensaje mensaje )
{
	objeto_post(handler->ptr_objeto2, mensaje);
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
	QMPool_put(&(handler->pool_memoria), handler->mensaje.ptr_datos - INDICE_INICIO_MENSAJE); // El inicio del bloque tiene como offset el INDICE_INICIO_MENSAJE

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
	tMensaje mensaje;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	uint8_t byte_recibido = uartRxRead( handler->uart ); // Leo byte de la UART con sAPI

	if (sf_recibir_byte(handler, byte_recibido))	// R_C2_5 Proceso el byte en contexto de interrupcion, si llego EOM devuelve true, sino devuelve false
	{
		if (sf_paquete_validar(handler))			// R_C2_10
		{
			// Cargo puntero con inicio de mensaje para la aplicación
			mensaje.ptr_datos = handler->buffer + INDICE_INICIO_MENSAJE;
			mensaje.cantidad = handler->cantidad - LEN_HEADER;
			// Envío a la cola el mensaje para la capa de aplicación.
			objeto_post_fromISR(handler->ptr_objeto1, mensaje, &xHigherPriorityTaskWoken); // R_C2_22
			sf_reiniciar_mensaje(handler);
			//  Pido un bloque de memoria nuevo, en caso de que no haya para la recepción por UART
			if (!sf_bloque_de_memoria_nuevo(handler))					// R_C2_8
			{
				// Prendo este flag para indicar que cuando se libere un bloque de memoria se pida uno nuevo y se vuelva a habilitar la recepcion
				handler->out_of_memory = true;
				sf_reception_set(handler, RECEPCION_DESACTIVADA ); 		// R_C2_9
			}
		}
		else
			sf_reiniciar_mensaje(handler);			// R_C2_12 y R_C2_21
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
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
	BaseType_t xTaskWokenByReceive = pdFALSE;

	if (indice_byte_enviado == 0)
		{
			objeto_get_fromISR(handler->ptr_objeto2, &handler->mensaje, &xTaskWokenByReceive);
			/* calculo el CRC del nuevo mensaje*/
			uint8_t crc = crc8_calc(0, handler->mensaje.ptr_datos - LEN_ID, handler->mensaje.cantidad + LEN_ID);
			// Paso a ascii el CRC
			itoa(crc,&(handler->mensaje.ptr_datos[handler->mensaje.cantidad]),16);
			// Inserto el EOM
			handler->mensaje.ptr_datos[handler->mensaje.cantidad + LEN_CRC] = EOM_BYTE;
		}
	
	if(handler->mensaje.cantidad != 0)
	{
		uartTxWrite(handler->uart, *(handler->mensaje.ptr_datos - INDICE_INICIO_MENSAJE + indice_byte_enviado) ); // R_C2_13 - R_C2_16
		indice_byte_enviado++;
		if ( indice_byte_enviado == (handler->mensaje.cantidad + LEN_HEADER))
		{
			indice_byte_enviado = 0;
			sf_bloque_de_memoria_liberar(handler); 			// R_C2_15
			//Verifico el flag, si se había quedado sin bloque de memoria pido uno ahora que liberé.
			if(handler->out_of_memory)
			{
				if (sf_bloque_de_memoria_nuevo(handler))
				{
					// Si consigo el bloque apago el flag y habilito la recepción. De lo contrario no hago nada
					handler->out_of_memory = false;
					sf_reception_set(handler, RECEPCION_ACTIVADA );
				}
			}
			handler->mensaje.cantidad = 0;
			handler->mensaje.ptr_datos = NULL;
			uartCallbackClr(handler->uart, UART_TRANSMITER_FREE); //Elimino el callback para parar la tx_isr
		}
	}
	portYIELD_FROM_ISR( xTaskWokenByReceive );
}

/**
 * @brief Timer callback.
 * 
 * @param xTimer    Timer handler.
 */
static void timer_callback(TimerHandle_t xTimer)
{
    sf_t* handler = (sf_t*)pvTimerGetTimerID(xTimer);
    if (xTimer == handler->timer)                       // R_C2_17
    {
        gpioWrite(GPIO0, OFF);                          // Para debug timer
        handler->cantidad = 0;
        handler->SOM = false;
    }
}
