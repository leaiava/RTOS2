/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v4.0
 *===========================================================================*/

#include "app.h"

static bool app_extraer_palabras(app_t* handler_app);
static bool app_procesar_mensaje(app_t* handler_app);
static void app_insertar_mensaje_error(app_t* handler_app);  
static void app_inicializar_array_palabras(app_t* handler_app);
static void app_camelCase(app_t* handler_app);
static void app_PascalCase(app_t* handler_app);
static void app_snake_case(app_t* handler_app);
void task_app(void* pvParameters)
{
	app_t* ptr_app = pvParameters;

	while(TRUE)
	{
		sf_mensaje_recibir( ptr_app->handler_sf , &(ptr_app->mensaje) );
		
        if (app_extraer_palabras(ptr_app) == 0)
            app_insertar_mensaje_error(ptr_app);    // Si salió con error, inserto el mensaje de error para ser enviado
        
        else if (app_procesar_mensaje(ptr_app) == 0)
            app_insertar_mensaje_error(ptr_app);    // Si salió con error, inserto el mensaje de error para ser enviado

        /* Envío el mensaje*/
		sf_mensaje_procesado_enviar(ptr_app->handler_sf, ptr_app->mensaje );
	}
}

/**
 * @brief Asigna memoria para una estructura de app, la inicializa y crea la tarea.
 * 
 * @param handler_app   Puntero del tipo app_t
 * @param handler_sf    Puntero del tipo sf_t, requiere que este inicializado.
 * @return true     Si salio todo bien
 * @return false    Si hubo un problema
 */
bool app_crear(app_t* handler_app , sf_t* handler_sf)
{
	if ( handler_sf != NULL )
    {
        handler_app = pvPortMalloc(sizeof(app_t));
        configASSERT(handler_app != NULL);
        
        handler_app->handler_sf = handler_sf;
        handler_app->error_type = SIN_ERROR;
        
        /* Inicio las palabras en cero */
        app_inicializar_array_palabras(handler_app);

        BaseType_t res;

        res = xTaskCreate(
            task_app,						 // Funcion de la tarea a ejecutar
            (const char *)"task_app", 		 // Nombre de la tarea como String amigable para el usuario
            configMINIMAL_STACK_SIZE * 2,	 // Cantidad de stack de la tarea
            handler_app,				 // Parametros de tarea
            tskIDLE_PRIORITY + 1,			 // Prioridad de la tarea
            0								 // Puntero a la tarea creada en el sistema
        );

        configASSERT(res == pdPASS);
        
        return true;
    }
    return false;
} 

/**
 * @brief Extrae las palabras del mensaje y las guarda en la estructura app_t
 * 
 *          También verifica que los caracteres del mensaje sean correctos llamando
 *          a la función app_validar_mensaje(app_t* handler_app , tMensaje *mensaje);
 * 
 * @param handler_app 
 * @return true     Si pudo extraer con éxito todas las palabras
 * @return false    Si hubo algún error, el error queda registrado dentro de la estructura app_t
 */
  
static bool app_extraer_palabras(app_t* handler_app)
{
    uint32_t palabra = PALABRA_INICIAL;
    uint32_t caracter = CARACTER_INICIAL;
    
    /* Si el caracter final es guion bajo o espacio salgo con error*/       // R_C3_9
    if ( (handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad -1] == ' ') || (handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad -1] == '_') )
        {
            handler_app->error_type = ERROR_INVALID_DATA;
            return false;
        }

    /* Recorro todo el mensaje para extraer las palabras y verifico que los caracteres sean válidos. */
    for ( uint32_t i = INDICE_CAMPO_DATOS; i < handler_app->mensaje.cantidad ; i++)
    {
        /* Si el caracter está entre 'A' y 'Z' lo convierto a minúscula y lo copio en el array de palabras. */
        if ( ('A' <= handler_app->mensaje.ptr_datos[i]) && ( handler_app->mensaje.ptr_datos[i]<= 'Z'))          // R_C3_7
            {
                /* Si no es la primer palabra o el primer carácter, cambio de palabra. */
                if ((caracter != CARACTER_INICIAL))
                {
                    /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                    if (caracter < CANT_LETRAS_MIN)
                    {
                        handler_app->error_type = ERROR_INVALID_DATA;
                        app_inicializar_array_palabras(handler_app);
                        return false;
                    }
                    palabra++;                   // Incremento para cambiar de palabra.
                    caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
                }
                /* Guardo caracter en minúscula. */
                handler_app->palabras[palabra][caracter] = handler_app->mensaje.ptr_datos[i] + A_MINUSCULA;
                caracter++;                  // Avanzo un caracter para la próxima iteración.
            }
        /* Si el caracter está entre 'a' y 'z' lo copio al array de palabras. */ 
        else if ( ('a' <= handler_app->mensaje.ptr_datos[i]) && (handler_app->mensaje.ptr_datos[i] <= 'z'))     // R_C3_7
        {
            handler_app->palabras[palabra][caracter] = handler_app->mensaje.ptr_datos[i];
            caracter++;                     // Avanzo un caracter para la próxima iteración.
        }
        /* Si el caracter es guion bajo o espacio cambio de palabra. */ 
        else if ( (handler_app->mensaje.ptr_datos[i] == '_' ) || (handler_app->mensaje.ptr_datos[i] == ' ' ) )
        {
            /* Si hay 2 guiones bajos o espacios seguidos salgo con error*/             // R_C3_8
            if (handler_app->mensaje.ptr_datos[i] == handler_app->mensaje.ptr_datos[i+1])
                {
                    handler_app->error_type = ERROR_INVALID_DATA;
                    app_inicializar_array_palabras(handler_app);
                    return false;
                }            
            /* Si el caracter no es el inicial salto de palabra.*/
            if (caracter != CARACTER_INICIAL)
            {
                /* Verifico que la palabra tenga la cantidad de letras mínimas*/    // R_C3_4
                if (caracter < CANT_LETRAS_MIN)
                {
                    handler_app->error_type = ERROR_INVALID_DATA;
                    app_inicializar_array_palabras(handler_app);
                    return false;
                }
                palabra++;                   // Incremento para cambiar de palabra.
                caracter = CARACTER_INICIAL; // Cambio al caracter inicial de la palabra.
            }
        }
        /* Si no era un caracter, o guion bajo o espacio, marco el error y salgo. */
        else
        {
            handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(handler_app);
            return false;
        }
        /* Si llegue a la cantidad máxima de palabras o caracteres, marco el error y salgo*/
        if ( (caracter > CANT_LETRAS_MAX) || (palabra == CANT_PALABRAS_MAX))
        {
            handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(handler_app);
            return false;
        }
    }
    /* Verifico que la  cantidad de palabras que llegaron se mayor que las mínimas definidas.*/ // R_C3_2
    if (palabra < CANT_PALABRAS_MIN -1)
        {
            handler_app->error_type = ERROR_INVALID_DATA;
            app_inicializar_array_palabras(handler_app);
            return false;
        }
        
    return true;
}

/**
 * @brief Tomas las palabras extraidas en la estructura y según el campo C del mensaje
 *          aplica el formato correspondiente guardando el mensaje dentro de la estructura app_t
 * 
 * @param handler_app 
 * @return true     Si el OPCODE era correcto
 * @return false    Si hubo error en el OPCODE
 */
static bool app_procesar_mensaje(app_t* handler_app)
{
    switch(handler_app->mensaje.ptr_datos[INDICE_CAMPO_C])           // R_C3_12
    {
        case 'C':                       // A camelCase
            app_camelCase(handler_app);    
            break; 
        
        case 'P':                       // A PascalCase
            app_PascalCase(handler_app);
            break; 
        
        case 'S':                       // A snake_case
            app_snake_case(handler_app);
            break; // Para salir del case.
        
        default:                                                // R_C3_6 - R_C3_11
        {
            handler_app->error_type = ERROR_INVALID_OPCODE;
            app_inicializar_array_palabras(handler_app);
            return false;            
        }
        
    }
    return true;
}

/**
 * @brief Pone en cero el array de palabras
 *          Se la llama al iniciar y si aparece un ERROR_INVALID_DATA para que no queden datos corruptos
 * 
 * @param handler_app 
 */
static void app_inicializar_array_palabras(app_t* handler_app)
{
    for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
        for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
        {
            handler_app->palabras[i][j] = 0;
        }
}

/**
 * @brief Inserta el mensaje de error dentro del campo de mensaje de app_t
 * 
 * @param handler_app 
 */
static void app_insertar_mensaje_error(app_t* handler_app)      // R_C3_13
{
    uint8_t pos = 0;
    handler_app->mensaje.ptr_datos[pos++] = 'E';   
    handler_app->mensaje.ptr_datos[pos++] = '0';  
    handler_app->mensaje.ptr_datos[pos++] = handler_app->error_type + '0';
    handler_app->mensaje.cantidad = 3;
    handler_app->error_type = SIN_ERROR;
}

/**
 * @brief 
 * 
 * @param handler_app 
 */
static void app_camelCase(app_t* handler_app)
        {
            handler_app->mensaje.cantidad = INDICE_CAMPO_DATOS; // Acá iniciará el campo de datos.

            /* Bucle para recorrer todas las palabras*/ 
            for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
            {
                if (handler_app->palabras[i][CARACTER_INICIAL] == 0 ) 
                    break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
                for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
                {
                    if (handler_app->palabras[i][j] == 0 ) 
                        break; //Si es cero, llegué al final de la palabra y salgo del for.
                    
                    /* A partir de la segunda palabra, pongo la primer letra de la palabra en mayúscula */
                    if ( (i > 0) && (j == 0))
                        handler_app->palabras[i][j] += A_MAYUSCULA;
                    
                    /* Cargo en el mensaje el caracter correspondiente*/ 
                    handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad] = handler_app->palabras[i][j];
                    /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
                    handler_app->palabras[i][j] = 0;
                    /* Incremento el tamaño del paquete*/
                    handler_app->mensaje.cantidad++;
                }
            }
        }

/**
 * @brief 
 * 
 * @param handler_app 
 */
static void app_PascalCase(app_t* handler_app)
        {
            handler_app->mensaje.cantidad = INDICE_CAMPO_DATOS; // Acá iniciará el campo de datos.

            /* Bucle para recorrer todas las palabras*/ 
            for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
            {
                if (handler_app->palabras[i][CARACTER_INICIAL] == 0 ) 
                    break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
                for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
                {
                    if (handler_app->palabras[i][j] == 0 ) 
                        break; //Si es cero, llegué al final de la palabra y salgo del for.
                    
                    /* Pongo la primer letra de la palabra en mayúscula */
                    if ( j == 0 )
                        handler_app->palabras[i][j] += A_MAYUSCULA;
                    
                    /* Cargo en el mensaje el caracter correspondiente*/ 
                    handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad] = handler_app->palabras[i][j];
                    /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
                    handler_app->palabras[i][j] = 0;
                    /* Incremento el tamaño del paquete*/
                    handler_app->mensaje.cantidad++;
                }
            }
        }

/**
 * @brief 
 * 
 * @param handler_app 
 */
static void app_snake_case(app_t* handler_app)
        {
            handler_app->mensaje.cantidad = INDICE_CAMPO_DATOS; // Acá iniciará el campo de datos.

            /* Bucle para recorrer todas las palabras*/ 
            for (uint32_t i = 0 ; i < CANT_PALABRAS_MAX ; i++)
            {
                if (handler_app->palabras[i][CARACTER_INICIAL] == 0 ) 
                    break;  // Si el primer caracter de la palabra es cero, significa que termine con todas las palabras y salgo del for
                for (uint32_t j = 0 ; j < CANT_LETRAS_MAX ; j++)
                {
                    if (handler_app->palabras[i][j] == 0 ) 
                        break; //Si es cero, llegué al final de la palabra y salgo del for.
                    
                    /* A partir de la segunda palabra, inserto un '_' al inicio */
                    if ( (i > 0) && (j == 0) )
                        {
                            handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad] = '_';
                            handler_app->mensaje.cantidad++;
                        }
                     
                    /* Cargo en el mensaje el caracter correspondiente*/ 
                    handler_app->mensaje.ptr_datos[handler_app->mensaje.cantidad] = handler_app->palabras[i][j];
                    /* Una vez cargado el caracter, lo reinicio para que el array palabras sea reutilizado*/
                    handler_app->palabras[i][j] = 0;
                    /* Incremento el tamaño del paquete*/
                    handler_app->mensaje.cantidad++;
                }
            }
}

/**
 * @brief Inserta el mensaje de error dentro del campo de mensaje de app_t
 * 
 * @param handler_app 
 */
static void app_insertar_mensaje_error(app_t* handler_app)      // R_C3_13
{
    uint8_t pos = 0;
    handler_app->mensaje.ptr_datos[pos++] = 'E';   
    handler_app->mensaje.ptr_datos[pos++] = '0';  
    handler_app->mensaje.ptr_datos[pos++] = handler_app->error_type + '0';
    handler_app->mensaje.cantidad = 3;
    handler_app->error_type = SIN_ERROR;
}
