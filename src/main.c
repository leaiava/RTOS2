/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 12/11/2021
 * Version: v2.0
 *===========================================================================*/

/*==================[inclusiones]============================================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include "sapi.h"
#include "separacion_frames.h"
#include "app.h"

/*==================[definiciones y macros]==================================*/
#define UART_USED	UART_USB
#define BAUD_RATE	115200

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

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

	res = xTaskCreate(
		task_app,						 // Funcion de la tarea a ejecutar
		(const char *)"task_app", 		 // Nombre de la tarea como String amigable para el usuario
		configMINIMAL_STACK_SIZE * 2,	 // Cantidad de stack de la tarea
		ptr_sf,							 // Parametros de tarea
		tskIDLE_PRIORITY + 1,			 // Prioridad de la tarea
		0								 // Puntero a la tarea creada en el sistema
	);

	configASSERT(res == pdPASS);

    vTaskStartScheduler();


    return 0;
}
/*==================[definiciones de funciones internas]=====================*/

/**
 * @brief	Manejo de errores
 */
void error_handler()
{

}
