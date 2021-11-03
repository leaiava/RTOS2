/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "sapi.h"
#include "separacion_frames.h"
/*==================[definiciones y macros]==================================*/
#define UART_USED	UART_USB
#define BAUD_RATE	115200
#define miDEBUG
/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/
void tarea_principal(void* pvParameters);
void error_handler();
/*==================[declaraciones de funciones externas]====================*/

/*==================[funcion principal]======================================*/

int main(void)
{
	/* Inicializar la placa */
    boardConfig();

    sf_t* ptr_sf = sf_crear();
    configASSERT(ptr_sf != NULL);

    if (!sf_init(ptr_sf, UART_USED, BAUD_RATE))
	    error_handler();

    BaseType_t res;

    res =xTaskCreate(
	   tarea_principal,                 // Funcion de la tarea a ejecutar
	   (const char *)"tarea_principal", // Nombre de la tarea como String amigable para el usuario
	   configMINIMAL_STACK_SIZE*2, 		// Cantidad de stack de la tarea
	   ptr_sf,                     		// Parametros de tarea
	   tskIDLE_PRIORITY+1,         		// Prioridad de la tarea
	   0                           		// Puntero a la tarea creada en el sistema
    );

    configASSERT(res = pdPASS);

    vTaskStartScheduler();


    return 0;
}
/*==================[definiciones de funciones internas]=====================*/

void tarea_principal(void* pvParameters)
{
	sf_t* handler = (sf_t*)pvParameters;
	while(TRUE)
	{
		sf_mensaje_recibir(handler);
		// validar mensaje recibido

		// Procesar mensaje

#ifdef miDEBUG
		// Mando a la UART para debug
		uartWriteString(UART_USED, handler->ptr_mensaje->datos);
#endif
		sf_mensajeProcesado_enviar(handler);
	}
}



/**
 * @brief	Manejo de errores
 */
void error_handler()
{

}
