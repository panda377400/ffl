#include "lpc.h"
#include "arm7_intf.h"
#include "burnint.h"          // 提供 UINT8, UINT16, UINT32, INT32 等类型定义
#include <stdio.h>

UINT8 kovgsyxp_d00000_ram[0x400]; // 0xd00000-0xd003ff
UINT16 m_kov_highlatch_arm_w = 0;
UINT16 m_kov_lowlatch_arm_w  = 0;
UINT16 m_kov_highlatch_68k_w = 0;
UINT16 m_kov_lowlatch_68k_w  = 0;
UINT32 m_kov_pll_lock_timer = 0;
UINT32 m_kov_pll_enabled    = 0;
UINT32 m_kov_pll_config     = 0;
UINT32 m_kov_i2c1sclh   = 0;
UINT32 m_kov_i2c1conset = 0;
UINT32 m_kov_i2c1dat    = 0;
UINT32 m_kov_i2c1_status = 0x08;
UINT32 m_kov_handshake_done = 0;
INT32 kov_arm7_cpu_initted = 0;
// ---------------- VIC registers (match lpc2132_vic.cpp) ----------------
UINT32 vic_irq_status = 0;
UINT32 vic_fiq_status = 0;
UINT32 vic_raw_intr   = 0;
UINT32 vic_int_select = 0;
UINT32 vic_int_enable = 0;
UINT32 vic_soft_int   = 0;
UINT32 vic_protection = 0;
INT32 kov_arm7_initted = 0;
UINT32 vic_vect_addr[16] = {0};
UINT32 vic_vect_cntl[16] = {0};

UINT32 vic_vect_addr_cur = 0;
UINT32 vic_def_vect_addr = 0;


// 这两个在同文件内一般是 static 的，先声明，避免顺序问题
UINT32 kov_vic_read_long(UINT32 address);
void kov_vic_write_long(UINT32 address, UINT32 data);

UINT32 kov_lpc2132_read_long(UINT32 address);
void kov_lpc2132_write_long(UINT32 address, UINT32 data);
static inline UINT32 vic_read_vector_address()
{
	UINT32 irq_pending = vic_int_enable & vic_raw_intr & ~vic_int_select;

	if (irq_pending == 0) {
		vic_vect_addr_cur = vic_def_vect_addr;
		return vic_vect_addr_cur;
	}

	for (INT32 i = 0; i < 16; i++) {
		if (vic_vect_cntl[i] & 0x20) {
			UINT32 irq_source = vic_vect_cntl[i] & 0x1f;
			if (irq_pending & (1U << irq_source)) {
				vic_vect_addr_cur = vic_vect_addr[i];
				return vic_vect_addr_cur;
			}
		}
	}

	vic_vect_addr_cur = vic_def_vect_addr;
	return vic_vect_addr_cur;
}

// VIC: update interrupt lines (align to MAME: level-sensitive ASSERT/CLEAR)
// -------------------------------------------
void vic_update_interrupt_lines()
{
    // 计算 pending
    UINT32 irq_pending = vic_int_enable & vic_raw_intr & ~vic_int_select;
    UINT32 fiq_pending = vic_int_enable & vic_raw_intr &  vic_int_select;

    vic_irq_status = irq_pending;
    vic_fiq_status = fiq_pending;

    // ---------------------------------------
    // FBNeo ARM7 IRQ 语义：
    // - AUTO: 脉冲（很容易“丢中断”）
    // - HOLD/ACK: 电平保持（更像 MAME ASSERT_LINE）
    //
    // 这里优先用 HOLD（或 ACK），如果项目里没有该宏则退化到 AUTO。
    // ---------------------------------------
#ifndef CPU_IRQSTATUS_HOLD
#define CPU_IRQSTATUS_HOLD CPU_IRQSTATUS_AUTO
#endif
#ifndef CPU_IRQSTATUS_ACK
#define CPU_IRQSTATUS_ACK CPU_IRQSTATUS_HOLD
#endif

    // 只要 pending 非 0，就保持拉高；否则清掉
    Arm7SetIRQLine(ARM7_IRQ_LINE,  irq_pending ? CPU_IRQSTATUS_HOLD : CPU_IRQSTATUS_NONE);
    Arm7SetIRQLine(ARM7_FIRQ_LINE, fiq_pending ? CPU_IRQSTATUS_HOLD : CPU_IRQSTATUS_NONE);
}

// VIC: set raw interrupt source line
// -------------------------------------------
void vic_set_irq(INT32 irqline, INT32 state)
{
    if (irqline < 0 || irqline >= 32) return;

    UINT32 mask = (1U << irqline);

    // state: 0=clear, nonzero=assert
    if (state) {
        // 如果已经是 1，就别重复写（减少不必要更新）
        if (vic_raw_intr & mask) return;
        vic_raw_intr |= mask;
    } else {
        if ((vic_raw_intr & mask) == 0) return;
        vic_raw_intr &= ~mask;
    }

    vic_update_interrupt_lines();
}

void kov_vic_update_irq()
{
	if (!kov_arm7_initted) return;

	UINT32 irq_pending = vic_int_enable & vic_raw_intr & ~vic_int_select;
	UINT32 fiq_pending = vic_int_enable & vic_raw_intr &  vic_int_select;

	// ✅ 与 vic_update_interrupt_lines() 保持一致
	vic_irq_status = irq_pending;
	vic_fiq_status = fiq_pending;

	// 触发 ARM7 中断
	if (irq_pending) {
		Arm7SetIRQLine(ARM7_IRQ_LINE, CPU_IRQSTATUS_AUTO);
	} else {
		Arm7SetIRQLine(ARM7_IRQ_LINE, CPU_IRQSTATUS_NONE);
	}
	
	if (fiq_pending) {
		Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_AUTO);
	} else {
		Arm7SetIRQLine(ARM7_FIRQ_LINE, CPU_IRQSTATUS_NONE);
	}
}

void kov_vic_set_source(INT32 line, INT32 state)
{
	if (!kov_arm7_initted) return;

	if (state) vic_raw_intr |=  (1U << line);
	else       vic_raw_intr &= ~(1U << line);

	kov_vic_update_irq();
}

UINT32 kov_vic_read_long(UINT32 address)
{
	// VIC base: 0xFFFFF000
	UINT32 off = address & 0xFFF;

	switch (off)
	{
		case 0x000: return vic_irq_status;               // VICIRQStatus
		case 0x004: return vic_fiq_status;               // VICFIQStatus
		case 0x008: return vic_raw_intr;                 // VICRawIntr
		case 0x00C: return vic_int_select;               // VICIntSelect
		case 0x010: return vic_int_enable;               // VICIntEnable
		case 0x018: return vic_soft_int;                 // VICSoftInt
		case 0x020: return vic_protection;               // VICProtection
		case 0x030: return vic_read_vector_address();    // VICVectAddr
		case 0x034: return vic_def_vect_addr;            // VICDefVectAddr
	}

	// VICVectAddr0-15
	if (off >= 0x100 && off < 0x140) {
		return vic_vect_addr[(off - 0x100) >> 2];
	}

	// VICVectCntl0-15
	if (off >= 0x200 && off < 0x240) {
		return vic_vect_cntl[(off - 0x200) >> 2];
	}

	return 0;
}

void kov_vic_write_long(UINT32 address, UINT32 data)
{
	UINT32 off = address & 0xFFF;

	switch (off)
	{
		case 0x00C: // VICIntSelect
			vic_int_select = data;
			vic_update_interrupt_lines();
		return;

		case 0x010: // VICIntEnable (OR)
			vic_int_enable |= data;
			vic_update_interrupt_lines();
		return;

		case 0x014: // VICIntEnClear
			vic_int_enable &= ~data;
			vic_update_interrupt_lines();
		return;

		case 0x018: // VICSoftInt (OR)
			vic_soft_int |= data;
			vic_raw_intr |= vic_soft_int;
			vic_update_interrupt_lines();
		return;

		case 0x01C: // VICSoftIntClear
			vic_soft_int &= ~data;
			vic_raw_intr &= ~data;
			vic_update_interrupt_lines();
		return;

		case 0x020: // VICProtection
			vic_protection = data;
		return;

		case 0x030: // VICVectAddr - EOI: MAME 里写任何值会清 IRQ16（握手用）
		{
			// ✅ 修改：清除当前处理的 IRQ16（如果正在处理）
			if (vic_raw_intr & (1U << 16)) {
				vic_raw_intr &= ~(1U << 16);
				vic_update_interrupt_lines();
			}
			
			// 如果是写回 VICVectAddr（中断处理完成），清除当前向量地址
			vic_vect_addr_cur = 0;
			return;
		}

		case 0x034: // VICDefVectAddr - 同时用于 handshake detection
			vic_def_vect_addr = data;
			if (data != 0 && !m_kov_handshake_done) {
				m_kov_handshake_done = 1;
				
			}
		return;
	}

	// VICVectAddr0-15
	if (off >= 0x100 && off < 0x140) {
		vic_vect_addr[(off - 0x100) >> 2] = data;
		return;
	}

	// VICVectCntl0-15
	if (off >= 0x200 && off < 0x240) {
		vic_vect_cntl[(off - 0x200) >> 2] = data;
		return;
	}
}

// ---- LPC2132 minimal @ 0xE0000000 ----
UINT32 kov_lpc2132_read_long(UINT32 address)
{

	// MAME 用 offset(=address>>2) 来判断
	UINT32 offset = address >> 2;

	// PLL block: (offset & 0x3FFFC0) == 0x7F000
	if ((offset & 0x3FFFC0) == 0x7F000) {
		switch (offset & 0x3F)
		{
			case 0x20: return m_kov_pll_enabled;
			case 0x21: return m_kov_pll_config;
			case 0x22:
			{
				UINT32 status = 0;

				if (m_kov_pll_enabled) {
					m_kov_pll_lock_timer++;
					if (m_kov_pll_lock_timer > 10) status |= 0x0400;
				}

				status |= (m_kov_pll_enabled & 0x03) << 8;
				status |= (m_kov_pll_config & 0x1F);
				return status;
			}
			case 0x23: return 0;
		}
	}

	// I2C1 block: (offset & 0x3FFFC0) == 0xA000
	if ((offset & 0x3FFFC0) == 0xA000) {
		switch (offset & 0x3F)
		{
			case 0x00: return m_kov_i2c1conset;
			case 0x01: return m_kov_i2c1_status;
			case 0x02: return m_kov_i2c1dat;
			case 0x04: return m_kov_i2c1sclh;
			default:   return 0;
		}
	}

	return 0;
}

void kov_lpc2132_write_long(UINT32 address, UINT32 data)
{
	UINT32 offset = address >> 2;

	// PLL block: (offset & 0x3FFFC0) == 0x7F000 or 0x7F040
	if ((offset & 0x3FFFC0) == 0x7F000 || (offset & 0x3FFFC0) == 0x7F040) {
		switch (offset & 0x7F)
		{
			case 0x20:
				m_kov_pll_enabled = data & 0x03;
				if (m_kov_pll_enabled) m_kov_pll_lock_timer = 0;
			break;

			case 0x21:
				m_kov_pll_config = data & 0xFF;
			break;

			case 0x23:
			break;

			case 0x50:
				// MAME：data & 0x04 -> CLEAR irq16
				if (data & 0x04) {
					vic_set_irq(16, 0);  // ✅ 这应该调用 vic_set_irq
				}
			break;
		}
	}

	// I2C1 block
	if ((offset & 0x3FFFC0) == 0xA000) {
		switch (offset & 0x3F)
		{
			case 0x00: // I2C1CONSET
			{
				m_kov_i2c1conset = data;

				// handshake done 后，CONSET 位会在 MAME 中触发 latch 交换
				if (m_kov_handshake_done && (data & 0x00200000) && !(data & 0x00400000)) {
					if (data & 0x04000000) {
						// high read-back to 68k
						m_kov_highlatch_arm_w = (m_kov_i2c1sclh >> 16) & 0xffff;

						vic_set_irq(16, 0);
					} else {
						// low read-back to 68k
						m_kov_lowlatch_arm_w = (m_kov_i2c1sclh >> 16) & 0xffff;
					}
				}

				if (m_kov_handshake_done && (data & 0x00400000) && !(data & 0x00200000)) {
					if (data & 0x04000000) {
						m_kov_i2c1sclh = (UINT32)m_kov_highlatch_68k_w << 16;
					} else {
						m_kov_i2c1sclh = (UINT32)m_kov_lowlatch_68k_w << 16;
					}
				}

				// I2C 状态机（照 MAME）
				if (data & 0x400000) m_kov_i2c1_status = 0x08;
				if (data & 0x200000) m_kov_i2c1_status = 0x10;
				if ((data & 0x600000) == 0x600000) m_kov_i2c1_status = 0x28;
			}
			break;

			case 0x04: // I2C1SCLH
				m_kov_i2c1sclh = data;
				m_kov_i2c1_status = 0x28;
			break;

			case 0x02: // I2C1DAT
				m_kov_i2c1dat = data;
			break;

			case 0x06:
			break;
		}
	}
}

static inline bool kov_is_vic_addr(UINT32 address)
{
    return (address & 0xFFFFF000) == 0x7FFFF000;
}

static inline bool kov_is_lpc_addr(UINT32 a)
{
	// 这里按你现有实现：0x60000000-0x601FFFFF + 0x601FC000-0x601FFFFF 等
	return ((a & 0xFFF00000) == 0x60000000) || ((a & 0xFFFF0000) == 0x601F0000);
}

UINT8 kov_arm7_read_byte(UINT32 address)
{
    UINT32 a = address & ~3U;
    UINT32 v = kov_is_vic_addr(address) ? kov_vic_read_long(a) : kov_lpc2132_read_long(a);
    return (v >> ((address & 3U) * 8U)) & 0xFF;
}

void kov_arm7_write_byte(UINT32 address, UINT8 data)
{
    UINT32 a = address & ~3U;
    UINT32 v = kov_is_vic_addr(address) ? kov_vic_read_long(a) : kov_lpc2132_read_long(a);

    UINT32 shift = (address & 3U) * 8U;
    v = (v & ~(0xFFu << shift)) | ((UINT32)data << shift);

    if (kov_is_vic_addr(address)) kov_vic_write_long(a, v);
    else                         kov_lpc2132_write_long(a, v);
}

UINT16 kov_arm7_read_word(UINT32 address)
{
    UINT32 a = address & ~3U;
    UINT32 v = kov_is_vic_addr(address) ? kov_vic_read_long(a) : kov_lpc2132_read_long(a);
    return (address & 2U) ? (UINT16)(v >> 16) : (UINT16)(v & 0xFFFF);
}
void kov_arm7_write_word(UINT32 address, UINT16 data)
{
    UINT32 a = address & ~3U;
    UINT32 v = kov_is_vic_addr(address) ? kov_vic_read_long(a) : kov_lpc2132_read_long(a);

    if (address & 2U) v = (v & 0x0000FFFFu) | ((UINT32)data << 16);
    else              v = (v & 0xFFFF0000u) | (UINT32)data;

    if (kov_is_vic_addr(address)) kov_vic_write_long(a, v);
    else                         kov_lpc2132_write_long(a, v);
}

UINT32 kov_arm7_read_long(UINT32 address)
{
    if (kov_is_vic_addr(address)) return kov_vic_read_long(address);
    return kov_lpc2132_read_long(address);
}

void kov_arm7_write_long(UINT32 address, UINT32 data)
{
    if (kov_is_vic_addr(address)) { kov_vic_write_long(address, data); return; }
    kov_lpc2132_write_long(address, data);
}