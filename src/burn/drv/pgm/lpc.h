
#include "burnint.h"          // 提供 UINT8, UINT16, UINT32, INT32 等类型定义
#include <stdio.h>

extern UINT8 kovgsyxp_d00000_ram[0x400];
extern UINT16 m_kov_highlatch_arm_w;
extern UINT16 m_kov_lowlatch_arm_w;
extern UINT16 m_kov_highlatch_68k_w;
extern UINT16 m_kov_lowlatch_68k_w;
extern UINT32 m_kov_pll_lock_timer;
extern UINT32 m_kov_pll_enabled;
extern UINT32 m_kov_pll_config;
extern UINT32 m_kov_i2c1sclh;
extern UINT32 m_kov_i2c1conset;
extern UINT32 m_kov_i2c1dat;
extern UINT32 m_kov_i2c1_status;
extern UINT32 m_kov_handshake_done;
extern INT32  kov_arm7_cpu_initted;
extern UINT32 vic_irq_status;
extern UINT32 vic_fiq_status;
extern UINT32 vic_raw_intr;
extern UINT32 vic_int_select;
extern UINT32 vic_int_enable;
extern UINT32 vic_soft_int;
extern UINT32 vic_protection;
extern INT32  kov_arm7_initted;
extern UINT32 vic_vect_addr[16];
extern UINT32 vic_vect_cntl[16];
extern UINT32 vic_vect_addr_cur;
extern UINT32 vic_def_vect_addr;

void vic_update_interrupt_lines();
void vic_set_irq(INT32 line, INT32 state);
UINT32 kov_lpc2132_read_long(UINT32 address);
void   kov_lpc2132_write_long(UINT32 address, UINT32 data);
UINT32 kov_vic_read_long(UINT32 address);
void   kov_vic_write_long(UINT32 address, UINT32 data);

UINT8 kov_arm7_read_byte(UINT32 address);
void kov_arm7_write_byte(UINT32 address, UINT8 data);
UINT16 kov_arm7_read_word(UINT32 address);
void kov_arm7_write_word(UINT32 address, UINT16 data);
UINT32 kov_arm7_read_long(UINT32 address);
void kov_arm7_write_long(UINT32 address, UINT32 data);
