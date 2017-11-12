#ifndef _LASER_CONTROLLING_H
#define _LASER_CONTROLLING_H

#include "stdint.h"

void init_laser_controlling(void);

void laser_turn_on(void);
void laser_turn_off(void);

void laser_dma_start(void);
void  laser_dma_stop(void);

void prepare_laser_line(uint8_t line_number);


#endif