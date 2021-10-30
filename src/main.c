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
#include "sapi.h"

/*==================[definiciones y macros]==================================*/

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/
void tickTask( void* pvParameters );

/*==================[declaraciones de funciones externas]====================*/
void onRx( void *noUsado );

/*==================[funcion principal]======================================*/

int main(void)
{
   /* Inicializar la placa */
   boardConfig();

   /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
   uartConfig(UART_USB, 115200);     
   // Seteo un callback al evento de recepcion y habilito su interrupcion
   uartCallbackSet(UART_USB, UART_RECEIVE, onRx, NULL);
   // Habilito todas las interrupciones de UART_USB
   uartInterrupt(UART_USB, true);
   
   xTaskCreate(
      tickTask,                     // Funcion de la tarea a ejecutar
      (const char *)"tickTask",     // Nombre de la tarea como String amigable para el usuario
      configMINIMAL_STACK_SIZE*2, // Cantidad de stack de la tarea
      0,                          // Parametros de tarea
      tskIDLE_PRIORITY+1,         // Prioridad de la tarea
      0                           // Puntero a la tarea creada en el sistema
   );

   vTaskStartScheduler();

   return 0;
}
/*==================[definiciones de funciones internas]=====================*/

void tickTask( void* pvParameters )
{
   while(TRUE) {
      // Una tarea muy bloqueante para demostrar que la interrupcion funcina
      gpioToggle(LEDB);
      vTaskDelay(1000/portTICK_RATE_MS);
   }
}

/*==================[definiciones de funciones externas]=====================*/
void onRx( void *noUsado )
{
   char c = uartRxRead( UART_USB );
   printf( "Recibimos <<%c>> por UART\r\n", c );
}

/*==================[fin del archivo]========================================*/
