/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 23/11/2021
 * Version: v3.0
 *===========================================================================*/

#include "app.h"

static void app_extraer_palabras(app_t* handler_app);
static bool app_validar_mensaje(app_t* handler_app);    // TODO: Ver si conviene pasarle la estructura o solo un char.
static void app_procesar_mensaje(app_t* handler_app);
static void app_insertar_mensaje_error(app_t* handler_app);  

void task_app(void* pvParameters)
{
	sf_t* handler = (sf_t*)pvParameters;
	app_t* ptr_app;

    ptr_app = app_crear();
    app_init(ptr_app);

	while(TRUE)
	{
		if(!sf_mensaje_recibir(handler, &ptr_app->mensaje ))
			error_handler();
        
        app_extraer_palabras(ptr_app);

        app_validar_mensaje(ptr_app);
        
        app_procesar_mensaje(ptr_app);

		sf_mensaje_procesado_enviar(handler, ptr_app->mensaje );
	}
}

/**
 * @brief Asigna memoria para una estructura de app y devuelve puntero a ella. 
 * 
 * @return app_t* 
 */
app_t* app_crear(void)
{
	app_t* me = pvPortMalloc(sizeof(app_t));
	configASSERT(me != NULL);
	return me;
} 

/**
 * @brief Inicializa la estructura app
 * 
 * @param handler_app 
 * @return true 
 * @return false 
 */
bool app_init(app_t* handler_app)
{

}

/**
 * @brief Extrae las palabras del mensaje y las guarda en su propia estructura
 * 
 *          También verifica que los caracteres del mensaje sean correctos llamando
 *          a la función app_validar_mensaje(app_t* handler_app , tMensaje *mensaje);
 * 
 * @param handler_app 
 */
static void app_extraer_palabras(app_t* handler_app)
{

}

/**
 * @brief Verifica que los caracteres sean letras, espacio o guion bajo "_"
 * 
 * @param handler_app 
 * @return true 
 * @return false 
 */
static bool app_validar_mensaje(app_t* handler_app)
{

}

/**
 * @brief Tomas las palabras extraidas en la estructura y según el campo C del mensaje
 *          aplica el formato correspondiente y lo guarda en
 * 
 * @param handler_app 
 */
static void app_procesar_mensaje(app_t* handler_app)
{
    switch(handler_app->mensaje.ptr_datos[0])
    {
        case 'C':
        {
            
            break;
        }
        case 'P':
        {

            break;
        }
        case 'S':
        {

            break;
        }
        default:
        {

            break;
        }
        
    }
    

}