
#ifndef PROTOCOL_H_
#define PROTOCOL_H_

/**
 * @brief estado de recpcion de data
 *
 */
typedef enum
{
	reception_not = 1,
	reception_init,
	reception_stop
}reception_st;

void protocol_init();

#endif
