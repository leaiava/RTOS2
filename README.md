# RTOS2 v1.0
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

La capa de separación de frames esta compuesta principalmente por 2 tareas y la rutina de atención de interrupción de la UART.

Tarea_recibir_paquete_de_UART: Esta tarea se encarga de pedir un bloque de memoria al pool y se queda esperando un semáforo el cual le indicará que hay un nuevo paquete recibido. Cuando recibe el semáforo valida el ID y el CRC y envía a la aplicación el mensaje a través de un objeto, que en este caso, es una cola de mensajes. Una vez enviado el mensaje pide un nuevo bloque de memoria al pool y repita la operación.

Rutina de interrupción: Recibe con sAPI los bytes de la UART y se fija que llegue el SOM y EOM. Cuando el paquete está completo libera un semaforo para la tarea_recibir_paquete_de_UART.

Aplicacion: Se queda esperando el mensaje por cola, cuando lo recibe lo procesa y lo devuelve a la capa de separacion de frame.

Tarea_enviar_paquete_por_UART: Recibe el mensaje procesado, lo envía por la UART y libera la memoria.

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
- [ ] R_C2_10
- [ ] R_C2_11
- [ ] R_C2_12
- [ ] R_C2_13
- [ ] R_C2_14
- [ ] R_C2_15
- [ ] R_C2_16
- [ ] R_C2_17
- [ ] R_C2_18
- [ ] R_C2_19
- [ ] R_C2_20
- [ ] R_C2_21
- [ ] R_C2_22

### Requerimientos Capa Aplicación (C3):
- [ ] R_C3_1
- [ ] R_C3_2
- [ ] R_C3_3
- [ ] R_C3_4
- [ ] R_C3_5
- [ ] R_C3_6
- [ ] R_C3_7
- [ ] R_C3_8
- [ ] R_C3_9
- [ ] R_C3_10
- [ ] R_C3_11
- [ ] R_C3_12
- [ ] R_C3_13
### Requerimientos Objeto :
- [ ] R_AO_1
- [ ] R_AO_2
- [ ] R_AO_3
- [ ] R_AO_4
- [ ] R_AO_5
- [ ] R_AO_6
- [ ] R_AO_7
- [ ] R_AO_8
- [ ] R_AO_9

### Requerimientos Opcionales:
- [ ] R_O_1
- [ ] R_O_2










