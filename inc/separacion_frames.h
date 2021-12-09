/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 12/11/2021
 * Version: v2.0
 *===========================================================================*/

#ifndef separacion_frames_H_
#define separacion_frames_H_

#include "sapi.h"
#include "crc8.h"
#include "objeto.h"
#include "qmpool.h"
#include "timers.h"
#include "sepa_frame_def.h"

typedef struct
{
    uartMap_t uart;                        ///< Nombre de la UART del LPC4337 a utilizar.
    uint32_t baudRate;                     ///< BaudRate seleccionado para la comunicacion serie.
    uint8_t *buffer;                       ///< Buffer para la recepción de bytes.
    uint32_t cantidad;                     ///< Cantidad de bytes recibidos.    
    bool SOM;                              ///< Flag para indicar si llego el SOM.
    bool EOM;                              ///< Flag para indicar si llego el EOM.
    bool out_of_memory;                    ///< Indica que se quedo sin bloque de memoria.
    tObjeto *ptr_objeto1;                  ///< Puntero al objeto usado para enviar el mensaje del driver a la aplicacion.
    tObjeto *ptr_objeto2;                  ///< Puntero al objeto usado para enviar el mensaje de la aplicacion al driver.
    tMensaje mensaje;                      ///< Mensaje a recibirse a través del objeto.
    void *prt_pool;                        ///< Puntero al pool de memoria.
    QMPool pool_memoria;                   ///< Memory pool (contienen la información que necesita la biblioteca qmpool.h)
    TimerHandle_t timerRx;                 ///< TimerRx
    TimerHandle_t timerTx;                 ///< TimerTx
    TickType_t periodo_timerRx;              ///< Periodo del timer
} sf_t;

sf_t* sf_crear(void);
bool sf_init(sf_t* handler, uartMap_t uart, uint32_t baudRate);

bool sf_mensaje_recibir(sf_t* handler, tMensaje* ptr_mensaje);
void sf_mensaje_procesado_enviar(sf_t* handler, tMensaje mensaje);

#endif /* separacion_frames_H_ */
