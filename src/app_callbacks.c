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

static void app_extraer_palabras( uint8_t (*palabras)[CANT_LETRAS_MAX] , tMensaje* mensaje );
static void app_inicializar_array_palabras(uint8_t (*palabras)[CANT_LETRAS_MAX]);
static bool app_validar_paquete( tMensaje* mensaje );
static void app_insertar_mensaje_error(uint8_t error_type, tMensaje* mensaje );

/**
 * @brief   Callback para el OA_app. Recibe dos tipos de evento, uno de paquete a procesar y otro de paquete procesado
 *          Cuando llega un paquete a procesar valida el paquete y si es correcto de acuerdo al campo C, deriva el 
 *          paquete al OA activo correspondiente para que lo procese. Si el OA no existe lo crea.
 *          Cuando llega un paquete procesado lo devuelve a la capa 2.
 * 
 * @param caller_ao             Estructura del OA
 * @param mensaje_a_procesar    Paquete con el mensaje a procesar.
 */
void app_OAapp( void* caller_ao, void* mensaje_a_procesar )
{
    app_t* ptr_me = (app_t*) caller_ao; // Recibo por herencia el puntero a la estructura app_t
    tMensaje* mensaje = (tMensaje*) mensaje_a_procesar;
    
    /* Verifico si es un evento proveniente del driver que signifique “llegó un paquete procesar”. */    // R_AO_2
    if ( mensaje->evento_tipo == PAQUETE)
    {
        if (app_validar_paquete( mensaje ) == false )                            // R_AO_3
        {
            app_insertar_mensaje_error( ERROR_INVALID_DATA , mensaje );
            xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
            sf_setOn_tx_isr(ptr_me->handler_sf);
        }
        else switch( mensaje->ptr_datos[INDICE_CAMPO_C] ) // R_C3_12
    	{
            case 'C':
            taskENTER_CRITICAL();
            if ( ptr_me->OA_C.itIsAlive == false )
            {
            	// Se crea el objeto activo, con el comando correspondiente y tarea asociada.       //R_AO_5 R_AO_6
				if( activeObjectOperationCreate( &ptr_me->OA_C , app_OAC, activeObjectTask, ptr_me->handler_sf->ptr_objeto1->cola ) == false )
                {
                    app_insertar_mensaje_error( ERROR_SYSTEM , mensaje ); // R_AO_9
                    xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
                    sf_setOn_tx_isr(ptr_me->handler_sf);
                    break;
                }
			}
            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( &ptr_me->OA_C, mensaje);
            taskEXIT_CRITICAL();

            break; 
            
            case 'P':                       // A PascalCase
            taskENTER_CRITICAL();
            if( ptr_me->OA_P.itIsAlive == false )
            {
                // Se crea el objeto activo, con el comando correspondiente y tarea asociada.       //R_AO_5 R_AO_6
                if( activeObjectOperationCreate( &ptr_me->OA_P , app_OAP, activeObjectTask, ptr_me->handler_sf->ptr_objeto1->cola ) == false )
                {
                    app_insertar_mensaje_error( ERROR_SYSTEM , mensaje ); // R_AO_9
                    xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
                    sf_setOn_tx_isr(ptr_me->handler_sf);
                    break;
                }
            }
            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( &ptr_me->OA_P, mensaje);
            taskEXIT_CRITICAL();

            break;
            
            case 'S':                       // A snake_case
            taskENTER_CRITICAL();
            if( ptr_me->OA_S.itIsAlive == false)
            {
                // Se crea el objeto activo, con el comando correspondiente y tarea asociada.       //R_AO_5 R_AO_6
                if( activeObjectOperationCreate( &ptr_me->OA_S , app_OAS, activeObjectTask, ptr_me->handler_sf->ptr_objeto1->cola ) == false )
                {
                    app_insertar_mensaje_error( ERROR_SYSTEM , mensaje ); // R_AO_9
                    xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
                    sf_setOn_tx_isr(ptr_me->handler_sf);
                    break;
                }
            }
            // Y enviamos el dato a la cola para procesar.
            activeObjectEnqueue( &ptr_me->OA_S, mensaje);
            taskEXIT_CRITICAL();

            break; // Para salir del case.
            
            default:                                                // R_C3_6 - R_C3_11
            {
                app_insertar_mensaje_error( ERROR_INVALID_OPCODE , mensaje );
                xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
                sf_setOn_tx_isr(ptr_me->handler_sf);
            }
            
        }
    }
    /* Verifico si el mensaje que llego es un evento con la respuesta procesada*/       //R_AO_2
    if ( mensaje->evento_tipo == RESPUESTA)
    {
    	xQueueSend( ptr_me->handler_sf->ptr_objeto2->cola, mensaje, 0 );
        sf_setOn_tx_isr(ptr_me->handler_sf);
    }
    
}

/**
 * @brief  Callback para el OA que se encarga de formatear el mensaje en camelCase
 * 
 * @param caller_ao             Estructura del OA
 * @param mensaje_a_procesar    Paquete con el mensaje a procesar.
 */
void app_OAC(void* caller_ao, void* mensaje_a_procesar)
{
    activeObject_t* ptr_me = (activeObject_t*)caller_ao;
    tMensaje* mensaje = (tMensaje*) mensaje_a_procesar;

    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);

    app_extraer_palabras( palabras , mensaje );

    mensaje->cantidad = INDICE_CAMPO_DATOS;
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
            mensaje->ptr_datos[mensaje->cantidad] = palabras[i][j];

            /* Incremento el tamaño del paquete*/
            mensaje->cantidad++;
            mensaje->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
    xQueueSend( ptr_me->responseQueue , mensaje, 0 );
}

/**
 * @brief  Callback para el OA que se encarga de formatear el mensaje en PascalCase
 * 
 * @param caller_ao             Estructura del OA
 * @param mensaje_a_procesar    Paquete con el mensaje a procesar.
 */
void app_OAP(void* caller_ao, void* mensaje_a_procesar)
{
    activeObject_t* ptr_me = (activeObject_t*)caller_ao;
    tMensaje* mensaje = (tMensaje*) mensaje_a_procesar;

    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);

    app_extraer_palabras( palabras , mensaje );

    mensaje->cantidad = INDICE_CAMPO_DATOS;
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
            mensaje->ptr_datos[mensaje->cantidad] = palabras[i][j];

            /* Incremento el tamaño del paquete*/
            mensaje->cantidad++;
            mensaje->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
    xQueueSend( ptr_me->responseQueue , mensaje, 0 );

}

/**
 * @brief  Callback para el OA que se encarga de formatear el mensaje en snake_case
 * 
 * @param caller_ao             Estructura del OA
 * @param mensaje_a_procesar    Paquete con el mensaje a procesar.
 */
void app_OAS(void* caller_ao, void* mensaje_a_procesar)
{
    activeObject_t* ptr_me = (activeObject_t*)caller_ao;
    tMensaje* mensaje = (tMensaje*) mensaje_a_procesar;

    uint8_t palabras[CANT_PALABRAS_MAX][CANT_LETRAS_MAX];  ///> Array de strings para extraer las palabras del mensaje

    app_inicializar_array_palabras(palabras);

    app_extraer_palabras( palabras , mensaje );
    
    mensaje->cantidad = INDICE_CAMPO_DATOS;
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
            	mensaje->ptr_datos[mensaje->cantidad] = '_';
            	mensaje->cantidad++;
                }

            /* Cargo en el mensaje el caracter correspondiente*/
            mensaje->ptr_datos[mensaje->cantidad] = palabras[i][j];

            /* Incremento el tamaño del paquete*/
            mensaje->cantidad++;
            mensaje->evento_tipo = RESPUESTA;
        }
    }
    // Y enviamos el dato a la cola para procesar.
    xQueueSend( ptr_me->responseQueue , mensaje, 0 );

}

/**
 * @brief Función interna para validar el paquete a nivel de C3
 * 
 * @param mensaje   Mensaje a validar
 * @return true     Si el mensaje es correcto
 * @return false    Si el mensaje es incorrecto
 */
static bool app_validar_paquete( tMensaje* mensaje )
{
    uint32_t palabra = PALABRA_INICIAL;
    uint32_t caracter = CARACTER_INICIAL;
    
    /* Si el caracter final es guion bajo o espacio salgo con error*/       // R_C3_9
    if ( (mensaje->ptr_datos[mensaje->cantidad -1] == ' ') || (mensaje->ptr_datos[mensaje->cantidad -1] == '_') )
            return false;
    
    /* Recorro todo el mensaje para verificar que los caracteres sean válidos. */
    for ( uint32_t i = INDICE_CAMPO_DATOS; i < mensaje->cantidad ; i++)
    {
        if ( ('A' <= mensaje->ptr_datos[i]) && ( mensaje->ptr_datos[i]<= 'Z'))          // R_C3_7
            {
                /* Si no es la primer palabra o el primer carácter, cambio de palabra. */
                if ((caracter != CARACTER_INICIAL))
                {
                    /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                    if (caracter < CANT_LETRAS_MIN)
                        return false;
                    palabra++;                   // Incremento para cambiar de palabra.
                    caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
                }
                caracter++;                  // Avanzo un caracter para la próxima iteración.
            }
        else if ( ('a' <= mensaje->ptr_datos[i]) && (mensaje->ptr_datos[i] <= 'z'))     // R_C3_7
            caracter++;                     // Avanzo un caracter para la próxima iteración.
        else if ( (mensaje->ptr_datos[i] == '_' ) || (mensaje->ptr_datos[i] == ' ' ) )
        {
            /* Si hay 2 guiones bajos o espacios seguidos salgo con error*/             // R_C3_8
            if (mensaje->ptr_datos[i] == mensaje->ptr_datos[i+1])
                    return false;
            /* Si el caracter no es el inicial salto de palabra.*/
            if (caracter != CARACTER_INICIAL)
            {
                /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                if (caracter < CANT_LETRAS_MIN)
                    return false;
                palabra++;                   // Incremento para cambiar de palabra.
                caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
            }
        }
        /* Si no era un caracter, o guion bajo o espacio, marco el error y salgo. */
        else
            return false;
        /* Si llegue a la cantidad máxima de palabras o caracteres, marco el error y salgo*/
        if ( (caracter > CANT_LETRAS_MAX) || (palabra == CANT_PALABRAS_MAX))
            return false;
    }
    /* Verifico que la  cantidad de palabras que llegaron se mayor que las mínimas definidas.*/ // R_C3_2
    if (palabra < CANT_PALABRAS_MIN -1)
            return false;
        
    return true;
}

/**
 * @brief Extrae las palabras del mensaje y las guarda en una matriz.
 * 
 * @param mensaje   Mensaje a procesar
 * @param palabras  Matriz para guardar las palabras
 */
static void app_extraer_palabras( uint8_t (*palabras)[CANT_LETRAS_MAX] , tMensaje* mensaje )
{
    uint32_t palabra = PALABRA_INICIAL;
    uint32_t caracter = CARACTER_INICIAL;
    
    /* Recorro todo el mensaje para extraer las palabras */
    for ( uint32_t i = INDICE_CAMPO_DATOS; i < mensaje->cantidad ; i++)
    {
        /* Si el caracter está entre 'A' y 'Z' lo convierto a minúscula y lo copio en el array de palabras. */
        if ( ('A' <= mensaje->ptr_datos[i]) && ( mensaje->ptr_datos[i]<= 'Z'))          // R_C3_7
            {
                /* Si no es la primer palabra o el primer carácter, cambio de palabra. */
                if ((caracter != CARACTER_INICIAL))
                {
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
            /* Si el caracter no es el inicial salto de palabra.*/
            if (caracter != CARACTER_INICIAL)
            {
                palabra++;                   // Incremento para cambiar de palabra.
                caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
            }
        }
    }
}

/**
 * @brief Inicializa la matriz de palabras.
 * 
 * @param palabras Matriz de palabras a inicializar 
 */
static void app_inicializar_array_palabras(uint8_t (*palabras)[CANT_LETRAS_MAX])
{
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            palabras[i][j] = 0;
        }
}

/**
 * @brief       Inserta el mensaje de erro
 * 
 * @param error_type    Tipo de error
 * @param mensaje       Puntero al mensaje donde insertar el error
 */
static void app_insertar_mensaje_error(uint8_t error_type, tMensaje* mensaje )      // R_C3_13
{
    uint8_t pos = 0;
    mensaje->ptr_datos[pos++] = 'E';
    mensaje->ptr_datos[pos++] = '0';  
    mensaje->ptr_datos[pos++] = error_type + '0';
    mensaje->cantidad = 3;
}
