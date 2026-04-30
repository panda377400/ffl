#ifndef IGS027A_H
#define IGS027A_H

#include "burnint.h"

/*
 * IRQ/FIQ delivery is driven directly through Arm7SetIRQLine().
 */

void igs027a_init();
void igs027a_exit();
void igs027a_reset();
void igs027a_timer_w(INT32 which, UINT8 data);
void igs027a_timer_update(INT32 cycles);
void igs027a_set_pending_irq(INT32 bit);
void igs027a_external_irq(INT32 state);
void igs027a_external_fiq(INT32 state);
UINT8 igs027a_irq_pending_r();
void igs027a_irq_enable_w(UINT8 data);
void igs027a_fiq_enable_w(UINT8 data);
void igs027a_scan(INT32 nAction);

#endif
