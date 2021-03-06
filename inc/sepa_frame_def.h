/*=============================================================================
 * Copyright (c) 2021, Fernando Prokopiuk <fernandoprokopiuk@gmail.com>
 * 					   Jonathan Cagua <jonathan.cagua@gmail.com>
 * 					   Leandro Arrieta <leandroarrieta@gmail.com>
 * All rights reserved.
 * License: Free
 * Date: 30/10/2021
 * Version: v1.0
 *===========================================================================*/

#ifndef SEPA_FRAME_DEF_H_
#define SEPA_FRAME_DEF_H_

#define MSG_MAX_SIZE            200     // R_C2_2
#define SOM_BYTE                '('
#define EOM_BYTE                ')'
#define INDICE_INICIO_MENSAJE   5       // El mensaje para la aplicación comienza en el 5to byte
#define INDICE_INICIO_ID        1
#define CANT_BYTE_FUERA_CRC     4
#define POOL_SIZE             	2000
#define UART_IE                 true

#define ASCII_9                 '9'
#define ASCII_0                 '0'
#define ASCII_A                 'A'
#define ASCII_F                 'F'


#define LEN_CRC                 2
#define LEN_EOM                 1
#define LEN_ID                  4
#define LEN_SOM                 1
#define LEN_HEADER              (LEN_CRC+LEN_EOM+LEN_ID+LEN_SOM)
#define LEN_C                   1
#define LEN_HEADER_COMPLETO     (LEN_HEADER + LEN_C)
#define POS_CRC_H               2
#define POS_CRC_L               3

#define SHIFT_4b               4 // SHIFT LEFT 4BITS

#define ASCII_TO_NUM            55

#define TIMEOUT_MS              4 // R_C2_19
#define TIMEOUT                 pdMS_TO_TICKS(TIMEOUT_MS)

#endif
