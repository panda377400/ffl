#pragma once
#include "burnint.h"

void  pgm2MemcardInit(UINT32 size);
void  pgm2MemcardExit();
void  pgm2MemcardReset();
void  pgm2MemcardSetPresent(INT32 present);
INT32 pgm2MemcardPresent();
void  pgm2MemcardSetPresentSlot(UINT32 slot, INT32 present);
INT32 pgm2MemcardPresentSlot(UINT32 slot);
INT32 pgm2MemcardAuthenticatedSlot(UINT32 slot);
INT32 pgm2MemcardDirtySlot(UINT32 slot);
UINT32 pgm2MemcardGetSize(UINT32 slot);
void  pgm2MemcardLoadDefault(const UINT8* data, UINT32 len);
INT32 pgm2MemcardCreateImage(UINT8* data, UINT32 len);
INT32 pgm2MemcardLoadImageSlot(UINT32 slot, const UINT8* data, UINT32 len);
INT32 pgm2MemcardSaveImageSlot(UINT32 slot, UINT8* data, UINT32 len);
void  pgm2MemcardEjectSlot(UINT32 slot);

UINT8 pgm2MemcardRead8(UINT32 addr);
void  pgm2MemcardWrite8(UINT32 addr, UINT8 data);
UINT8 pgm2MemcardReadProt(UINT32 addr);
void  pgm2MemcardWriteProt(UINT32 addr, UINT8 data);
UINT8 pgm2MemcardReadSec(UINT32 addr);
void  pgm2MemcardWriteSec(UINT32 addr, UINT8 data);
void  pgm2MemcardAuth(UINT8 p1, UINT8 p2, UINT8 p3);

UINT8 pgm2MemcardRead8Slot(UINT32 slot, UINT32 addr);
void  pgm2MemcardWrite8Slot(UINT32 slot, UINT32 addr, UINT8 data);
UINT8 pgm2MemcardReadProtSlot(UINT32 slot, UINT32 addr);
void  pgm2MemcardWriteProtSlot(UINT32 slot, UINT32 addr, UINT8 data);
UINT8 pgm2MemcardReadSecSlot(UINT32 slot, UINT32 addr);
void  pgm2MemcardWriteSecSlot(UINT32 slot, UINT32 addr, UINT8 data);
void  pgm2MemcardAuthSlot(UINT32 slot, UINT8 p1, UINT8 p2, UINT8 p3);

void  pgm2MemcardScan(INT32 nAction);
