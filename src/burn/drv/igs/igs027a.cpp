#include "igs027a.h"

#include "arm7_intf.h"

static UINT8 igs027a_irq_pending;
static UINT8 igs027a_irq_enable;
static UINT8 igs027a_fiq_enable;
static UINT8 igs027a_ext_irq;
static UINT8 igs027a_ext_fiq;
static INT32 igs027a_timer_rate[2];
static INT32 igs027a_timer_period[2];
static INT32 igs027a_timer_count[2];

static inline void igs027a_update_irq_line()
{
	UINT8 active = (((~igs027a_irq_pending) & (~igs027a_irq_enable)) & 0xff) != 0;
	Arm7SetIRQLine(ARM7_IRQ_LINE, active ? CPU_IRQSTATUS_ACK : CPU_IRQSTATUS_NONE);
}

static inline void igs027a_assert_irq()
{
	igs027a_update_irq_line();
}

static inline void igs027a_clear_irq()
{
	igs027a_update_irq_line();
}

static inline void igs027a_pulse_fiq()
{
	Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_ACK);
	Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_NONE);
}

static inline void igs027a_clear_fiq()
{
	Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_NONE);
}

void igs027a_init()
{
	igs027a_irq_pending = 0;
	igs027a_irq_enable = 0;
	igs027a_fiq_enable = 0;
	igs027a_ext_irq = 0;
	igs027a_ext_fiq = 0;
	igs027a_timer_rate[0] = igs027a_timer_rate[1] = 0;
	igs027a_timer_period[0] = igs027a_timer_period[1] = 0;
	igs027a_timer_count[0] = igs027a_timer_count[1] = 0;
}

void igs027a_exit()
{
	igs027a_init();
}

void igs027a_reset()
{
	igs027a_irq_pending = 0xff;
	igs027a_irq_enable = 0xff;
	igs027a_fiq_enable = 0x01;
	igs027a_ext_irq = 0;
	igs027a_ext_fiq = 0;
	igs027a_timer_rate[0] = igs027a_timer_rate[1] = 0;
	igs027a_timer_period[0] = igs027a_timer_period[1] = 0;
	igs027a_timer_count[0] = igs027a_timer_count[1] = 0;
}

void igs027a_timer_w(INT32 which, UINT8 data)
{
	if (which < 0 || which > 1) return;

	igs027a_timer_rate[which] = data;
	igs027a_timer_period[which] = data ? (4263 * (data + 1)) : 0;
	igs027a_timer_count[which] = 0;
}

void igs027a_timer_update(INT32 cycles)
{
	for (INT32 i = 0; i < 2; i++) {
		if (igs027a_timer_period[i] == 0) continue;

		igs027a_timer_count[i] += cycles;
		while (igs027a_timer_count[i] >= igs027a_timer_period[i]) {
			igs027a_timer_count[i] -= igs027a_timer_period[i];
			igs027a_set_pending_irq(i);
		}
	}
}

void igs027a_set_pending_irq(INT32 bit)
{
	if ((igs027a_irq_enable & (1 << bit)) == 0) {
		igs027a_irq_pending &= ~(1 << bit);
		igs027a_assert_irq();
	}
}

void igs027a_external_irq(INT32 state)
{
	if (state && !igs027a_ext_irq && ((igs027a_irq_enable & (1 << 3)) == 0)) {
		igs027a_irq_pending &= ~(1 << 3);
		igs027a_assert_irq();
	}

	igs027a_ext_irq = state ? 1 : 0;
}

void igs027a_external_fiq(INT32 state)
{
	if (state && !igs027a_ext_fiq && (igs027a_fiq_enable & 1)) {
		igs027a_pulse_fiq();
	}

	igs027a_ext_fiq = state ? 1 : 0;
}

UINT8 igs027a_irq_pending_r()
{
	UINT8 ret = igs027a_irq_pending;
	igs027a_irq_pending = 0xff;
	igs027a_clear_irq();
	return ret;
}

void igs027a_irq_enable_w(UINT8 data)
{
	igs027a_irq_enable = data;
	igs027a_update_irq_line();
}

void igs027a_fiq_enable_w(UINT8 data)
{
	UINT8 old = igs027a_fiq_enable;
	igs027a_fiq_enable = data;

	if (((~old) & data & 1) != 0) {
		if (igs027a_ext_fiq && (igs027a_fiq_enable & 1)) {
			igs027a_pulse_fiq();
		}
	} else if (((old & ~data) & 1) != 0) {
		igs027a_clear_fiq();
	}
}

void igs027a_scan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(igs027a_irq_pending);
		SCAN_VAR(igs027a_irq_enable);
		SCAN_VAR(igs027a_fiq_enable);
		SCAN_VAR(igs027a_ext_irq);
		SCAN_VAR(igs027a_ext_fiq);
		SCAN_VAR(igs027a_timer_rate);
		SCAN_VAR(igs027a_timer_period);
		SCAN_VAR(igs027a_timer_count);
	}
}
