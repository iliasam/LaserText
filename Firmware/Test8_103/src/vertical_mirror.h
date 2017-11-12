#ifndef _VERTICAL_MIRROR_H
#define _VERTICAL_MIRROR_H

void vertical_mirror_init_hardware(void);

void vertical_mirror_set_pwm_a(uint16_t value);
void vertical_mirror_set_pwm_b(uint16_t value);

void vertical_mirror_new_step(void);

#endif