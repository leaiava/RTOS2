# RTOS2 v4.0
Repositorio para el Trabajo Práctico de la asignatura Sistemas Operativos en Tiempo Real 2 de la Carrera de Especialización en Sistemas Embebidos.

# Miembros del grupo 3
- Leando Arrieta <leandroarrieta@gmail.com>
- Fernando Andres Prokopiuk <fernandoprokopiuk@gmail.com>
- Jonathan Cagua Ordonez <jonathan.cagua@gmail.com>

# Objetivo
El objetivo de esta práctica es aprender a trabajar con algoritmos de asignación de memoria en un contexto de ejecución de tiempo real en donde se debe aprovechar al máximo la utilización del CPU.

Además, el trabajo práctico permite aprender a trabajar con procesamiento asincrónico de eventos.

# Justificaciones

## Esquema de memoria dinámica

Utilizamos la biblioteca qf_mem.c para el manejo de la memoria dinámica.

Se utilizan bloques de memoria del tamaño máximo de los paquetes que se pueden recibir. La capa de separación de frames es la encargada de pedir un bloque de memoria al pool, guardar en contexto de interrupción los bytes recibidos en ese bloque y de pasarle el mensaje a la aplicación para que lo procese. Una vez que la capa de separación de frame recibe el mensaje procesado vuelve a enviar el paquete por la UART y libera el bloque de memoria. La capa de separación de frames, mientras espera la respuesta de la aplicación, pide un nuevo bloque de memoria al pool para poder recibir otro paquete por UART.

## Arquitectura del sistema

La capa de separación de frames esta compuesta principalmente por la función de recibir mensaje y las rutinas de atención de interrupción de RX y TX de la UART.

Función de recibir mensaje: se encarga de pedir un bloque de memoria al pool, de habilitar la recepción por UART cuando lo obtiene. Y se queda esperando que se publique un mensaje en la cola de objeto 1.

Rutina de interrupción de recepción: recibe con sAPI los bytes de la UART y se fija que llegue el SOM y EOM. Cuando el paquete está completo y es válido, manda el paquete por la cola de objeto 1.

Aplicación: Se implementa la aplicación con un objeto activo que se encarga de recibir 2 tipo de evento, uno con los paquetes a procesar de capa 2 y otro con paquetes procesados. 

Al recibir un evento con un paquete para procesar verifica que su contenido sea correcto y deriva el procesamiento a otro objeto activo.

Al recibir un evento con un paquete procesado lo devuelve a capa 2.

Existira un objeto activo de procesamiento por formato que puede convertir la aplicación, en este caso tendremos 3 (camelCase, PascalCase y snake_case).

Rutina de interrupción de transmisión: obtiene el mensaje procesado la cola de objeto 2, lo envía por la UART y libera la memoria.


# Requerimientos
[Lista de requerimientos.](https://docs.google.com/spreadsheets/d/1-VyaQY0eDLpg12Eqkxe7_bfCb77LKIbDfVTNDGFBpu0/edit?usp=sharing)

### Requerimientos Generales:
- [x] R_TP_1
- [x] R_TP_2
- [x] R_TP_3
- [x] R_TP_4

### Requerimientos Capa Separación de frames (C2):
- [x] R_C2_1
- [x] R_C2_2
- [x] R_C2_3
- [x] R_C2_4
- [x] R_C2_5
- [x] R_C2_6
- [x] R_C2_7
- [x] R_C2_8
- [x] R_C2_9
- [x] R_C2_10
- [x] R_C2_11
- [x] R_C2_12
- [x] R_C2_13
- [x] R_C2_14
- [x] R_C2_15
- [x] R_C2_16
- [x] R_C2_17
- [x] R_C2_18
- [x] R_C2_19
- [x] R_C2_20
- [x] R_C2_21
- [x] R_C2_22

### Requerimientos Capa Aplicación (C3):
- [x] R_C3_1
- [x] R_C3_2
- [x] R_C3_3
- [x] R_C3_4
- [x] R_C3_5
- [x] R_C3_6
- [x] R_C3_7
- [x] R_C3_8
- [x] R_C3_9
- [x] R_C3_10
- [x] R_C3_11
- [x] R_C3_12
- [x] R_C3_13
### Requerimientos Objeto :
- [x] R_AO_1
- [x] R_AO_2
- [x] R_AO_3
- [x] R_AO_4
- [x] R_AO_5
- [x] R_AO_6
- [x] R_AO_7
- [x] R_AO_8
- [x] R_AO_9

### Requerimientos Opcionales:
- [ ] R_O_1
- [ ] R_O_2










