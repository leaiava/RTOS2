/**
 * @file protocol.c
 * @author 	Leando Arrieta leandroarrieta@gmail.com
 *			Fernando Andres Prokopiuk fernandoprokopiuk@gmail.com
 *			Jonathan Cagua Ordonez jonathan.cagua@gmail.com
 * @brief recepcion de datas por puerto serial y analisis de frame
 * @version 0.1
 * @date 2021-10-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "protocol.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"

#define FRAME_LEN_MAX 	200
#define UART_TO_USE		UART_GPIO
#define FRAME_START		0x28// es equivalente a (
#define FRAME_STOP		0x29// es equivalente a )

uint8_t frame_rx[FRAME_LEN_MAX];
uint8_t	frame_pos = 0;
reception_st recep_status= false;
SemaphoreHandle_t mutex;

static void protocol_rx_callback(void *pvParameters){
	/* leemos el caracter recibido */
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	char data = uartRxRead( UART_TO_USE );

	xSemaphoreTakeFromISR( mutex, &xHigherPriorityTaskWoken );
	if(FRAME_LEN_MAX -1 == frame_pos){
		frame_pos = 0;//ver si es q no se llego al maximo
	}
	if(FRAME_START == data){
		frame_pos = 0;
		recep_status = reception_init;
	}
	if(FRAME_STOP == data){
		if(reception_init == recep_status){
			recep_status = reception_stop;
		}
	}
	frame_rx[frame_pos++] = data;

	xSemaphoreGiveFromISR(mutex, &xHigherPriorityTaskWoken );
}

void protocol_init(){
	   /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
	   uartConfig(UART_TO_USE, 115200);
	   // Seteo un callback al evento de recepcion y habilito su interrupcion
	   uartCallbackSet(UART_TO_USE, UART_RECEIVE, protocol_rx_callback, NULL);
	   // Habilito todas las interrupciones de UART_USB
	   uartInterrupt(UART_TO_USE, true);

	   mutex = xSemaphoreCreateMutex();
	   configASSERT( mutex  != NULL );
}
