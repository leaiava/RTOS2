/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 29/11/2021
 * Version: v1.0
 *===========================================================================*/

#include "app_callbacks.h"
#include "app.h"

static bool app_extraer_palabras( uint8_t (*palabras)[CANT_LETRAS_MAX] , tMensaje* mensaje );
static void app_inicializar_array_palabras(uint8_t (*palabras)[CANT_LETRAS_MAX]);

void app_OAapp( void* caller_ao, void* mensaje_a_procesar )
{

    if ( ((tMensaje*)mensaje_a_procesar)->evento_tipo == PAQUETE)
    {
        // TODO: Chequear paquete
     
        switch( ((tMensaje*)mensaje_a_procesar)->ptr_datos[INDICE_CAMPO_C] ) // R_C3_12
    	{
            case 'C':
            if ( ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_C)->itIsAlive == false )
            {
            	// Se crea el objeto activo, con el comando correspondiente y tarea asociada.
				activeObjectOperationCreate( (activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_C , app_OAC, activeObjectTask, ((activeObject_t*)caller_ao)->handler_app->handler_sf->ptr_objeto1->cola );
			}

            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_C), mensaje_a_procesar);
                                
            break; 
            
            case 'P':                       // A PascalCase
            if(  ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_P)->itIsAlive == false )
				{
					// Se crea el objeto activo, con el comando correspondiente y tarea asociada.
					activeObjectOperationCreate( (activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_P , app_OAP, activeObjectTask, ((activeObject_t*)caller_ao)->handler_app->handler_sf->ptr_objeto1->cola );
				}

            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_P), mensaje_a_procesar);

            break;
            
            case 'S':                       // A snake_case
            if(  ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_S)->itIsAlive == false)
				{
					// Se crea el objeto activo, con el comando correspondiente y tarea asociada.
            		activeObjectOperationCreate( (activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_S , app_OAS, activeObjectTask, ((activeObject_t*)caller_ao)->handler_app->handler_sf->ptr_objeto1->cola );
				}

            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( ((activeObject_t*)((activeObject_t*)caller_ao)->ptr_OA_S), mensaje_a_procesar);

            break; // Para salir del case.
            
            default:                                                // R_C3_6 - R_C3_11
            {
                //handler_app->error_type = ERROR_INVALID_OPCODE;
                //app_inicializar_array_palabras(handler_app);
                //return false;
            }
            
        }
    }
    if ( ((tMensaje*)mensaje_a_procesar)->evento_tipo == RESPUESTA)
    {
    	activeObjectEnqueueResponse( (activeObject_t*)caller_ao ,  mensaje_a_procesar );
    	uartCallbackSet(((activeObject_t*)caller_ao )->handler_app->handler_sf->uart, UART_TRANSMITER_FREE, sf_tx_isr, ((activeObject_t*)caller_ao )->handler_app->handler_sf);
    	uartSetPendingInterrupt(((activeObject_t*)caller_ao )->handler_app->handler_sf->uart);
    }
    //return true;
}
void app_OAC(void* caller_ao, void* mensaje_a_procesar)
{
    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);
    app_extraer_palabras( palabras , (tMensaje*) mensaje_a_procesar ); //TODO: Analizar e inyectar el error

    ((tMensaje*) mensaje_a_procesar)->cantidad = INDICE_CAMPO_DATOS;
    /* Bucle para recorrer todas las palabras*/ 
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
    {
        if (palabras[i][CARACTER_INICIAL] == 0 ) 
            break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            if (palabras[i][j] == 0 ) 
                break; //Si es cero, llegué al final de la palabra y salgo del for.
            
            /* A partir de la segunda palabra, pongo la primer letra de la palabra en mayúscula */
            if ( (i > 0) && (j == 0))
                palabras[i][j] += A_MAYUSCULA;
            
            /* Cargo en el mensaje el caracter correspondiente*/ 
            ((tMensaje*) mensaje_a_procesar)->ptr_datos[((tMensaje*) mensaje_a_procesar)->cantidad] = palabras[i][j];
            /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
            palabras[i][j] = 0; // TODO: Esto creo no hace falta. Estoy quemado para razonarlo ahora.
            /* Incremento el tamaño del paquete*/
            ((tMensaje*) mensaje_a_procesar)->cantidad++;
            ((tMensaje*) mensaje_a_procesar)->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
activeObjectEnqueueResponse( (activeObject_t*)caller_ao ,  mensaje_a_procesar );

}

void app_OAP(void* caller_ao, void* mensaje_a_procesar)
{
    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);
    app_extraer_palabras( palabras , (tMensaje*) mensaje_a_procesar ); //TODO: Analizar e inyectar el error

    ((tMensaje*) mensaje_a_procesar)->cantidad = INDICE_CAMPO_DATOS;
    /* Bucle para recorrer todas las palabras*/
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
    {
        if (palabras[i][CARACTER_INICIAL] == 0 )
            break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            if (palabras[i][j] == 0 )
                break; //Si es cero, llegué al final de la palabra y salgo del for.

            /* Pongo la primer letra de la palabra en mayúscula */
            if ( j == 0 )
                palabras[i][j] += A_MAYUSCULA;

            /* Cargo en el mensaje el caracter correspondiente*/
            ((tMensaje*) mensaje_a_procesar)->ptr_datos[((tMensaje*) mensaje_a_procesar)->cantidad] = palabras[i][j];
            /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
            palabras[i][j] = 0; // TODO: Esto creo no hace falta. Estoy quemado para razonarlo ahora.
            /* Incremento el tamaño del paquete*/
            ((tMensaje*) mensaje_a_procesar)->cantidad++;
            ((tMensaje*) mensaje_a_procesar)->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
    activeObjectEnqueueResponse( (activeObject_t*)caller_ao ,  mensaje_a_procesar );

}

void app_OAS(void* caller_ao, void* mensaje_a_procesar)
{
    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);
    app_extraer_palabras( palabras , (tMensaje*) mensaje_a_procesar ); //TODO: Analizar e inyectar el error

    ((tMensaje*) mensaje_a_procesar)->cantidad = INDICE_CAMPO_DATOS;
    /* Bucle para recorrer todas las palabras*/
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
    {
        if (palabras[i][CARACTER_INICIAL] == 0 )
            break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            if (palabras[i][j] == 0 )
                break; //Si es cero, llegué al final de la palabra y salgo del for.

            /* A partir de la segunda palabra, inserto un '_' al inicio */
            if ( (i > 0) && (j == 0) )
                {
            	 ((tMensaje*) mensaje_a_procesar)->ptr_datos[((tMensaje*) mensaje_a_procesar)->cantidad] = '_';
                    ((tMensaje*) mensaje_a_procesar)->cantidad++;
                }

            /* Cargo en el mensaje el caracter correspondiente*/
            ((tMensaje*) mensaje_a_procesar)->ptr_datos[((tMensaje*) mensaje_a_procesar)->cantidad] = palabras[i][j];
            /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
            palabras[i][j] = 0; // TODO: Esto creo no hace falta. Estoy quemado para razonarlo ahora.
            /* Incremento el tamaño del paquete*/
            ((tMensaje*) mensaje_a_procesar)->cantidad++;
            ((tMensaje*) mensaje_a_procesar)->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
    activeObjectEnqueueResponse( (activeObject_t*)caller_ao ,  mensaje_a_procesar );

}

static bool app_extraer_palabras( uint8_t (*palabras)[CANT_LETRAS_MAX] , tMensaje* mensaje )
{
    uint32_t palabra = PALABRA_INICIAL;
    uint32_t caracter = CARACTER_INICIAL;
    
    /* Si el caracter final es guion bajo o espacio salgo con error*/       // R_C3_9
    if ( (mensaje->ptr_datos[mensaje->cantidad -1] == ' ') || (mensaje->ptr_datos[mensaje->cantidad -1] == '_') )
        {
            //handler_app->error_type = ERROR_INVALID_DATA;
            return false;
        }

    /* Recorro todo el mensaje para extraer las palabras y verifico que los caracteres sean válidos. */
    for ( uint32_t i = INDICE_CAMPO_DATOS; i < mensaje->cantidad ; i++)
    {
        /* Si el caracter está entre 'A' y 'Z' lo convierto a minúscula y lo copio en el array de palabras. */
        if ( ('A' <= mensaje->ptr_datos[i]) && ( mensaje->ptr_datos[i]<= 'Z'))          // R_C3_7
            {
                /* Si no es la primer palabra o el primer carácter, cambio de palabra. */
                if ((caracter != CARACTER_INICIAL))
                {
                    /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                    if (caracter < CANT_LETRAS_MIN)
                    {
                        //handler_app->error_type = ERROR_INVALID_DATA;
                        app_inicializar_array_palabras(palabras);
                        return false;
                    }
                    palabra++;                   // Incremento para cambiar de palabra.
                    caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
                }
                /* Guardo caracter en minúscula. */
                palabras[palabra][caracter] = mensaje->ptr_datos[i] + A_MINUSCULA;
                caracter++;                  // Avanzo un caracter para la próxima iteración.
            }
        /* Si el caracter está entre 'a' y 'z' lo copio al array de palabras. */ 
        else if ( ('a' <= mensaje->ptr_datos[i]) && (mensaje->ptr_datos[i] <= 'z'))     // R_C3_7
        {
            palabras[palabra][caracter] = mensaje->ptr_datos[i];
            caracter++;                     // Avanzo un caracter para la próxima iteración.
        }
        /* Si el caracter es guion bajo o espacio cambio de palabra. */ 
        else if ( (mensaje->ptr_datos[i] == '_' ) || (mensaje->ptr_datos[i] == ' ' ) )
        {
            /* Si hay 2 guiones bajos o espacios seguidos salgo con error*/             // R_C3_8
            if (mensaje->ptr_datos[i] == mensaje->ptr_datos[i+1])
                {
                    //handler_app->error_type = ERROR_INVALID_DATA;
                    app_inicializar_array_palabras(palabras);
                    return false;
                }            
            /* Si el caracter no es el inicial salto de palabra.*/
            if (caracter != CARACTER_INICIAL)
            {
                /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                if (caracter < CANT_LETRAS_MIN)
                {
                    //handler_app->error_type = ERROR_INVALID_DATA;
                    app_inicializar_array_palabras(palabras);
                    return false;
                }
                palabra++;                   // Incremento para cambiar de palabra.
                caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
            }
        }
        /* Si no era un caracter, o guion bajo o espacio, marco el error y salgo. */
        else
        {
            //handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(palabras);
            return false;
        }
        /* Si llegue a la cantidad máxima de palabras o caracteres, marco el error y salgo*/
        if ( (caracter > CANT_LETRAS_MAX) || (palabra == CANT_PALABRAS_MAX))
        {
            //handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(palabras);
            return false;
        }
    }
    /* Verifico que la  cantidad de palabras que llegaron se mayor que las mínimas definidas.*/ // R_C3_2
    if (palabra < CANT_PALABRAS_MIN -1)
        {
            //handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(palabras);
            return false;
        }
        
    return true;
}

static void app_inicializar_array_palabras(uint8_t (*palabras)[CANT_LETRAS_MAX])
{
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            palabras[i][j] = 0;
        }
}
