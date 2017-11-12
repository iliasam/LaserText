#ifndef _MAIN_H
#define _MAIN_H

extern volatile uint32_t ms_counter;

#define START_TIMER(x, duration)  (x = (ms_counter + duration))
#define TIMER_ELAPSED(x)  ((ms_counter > x) ? 1 : 0)

#endif