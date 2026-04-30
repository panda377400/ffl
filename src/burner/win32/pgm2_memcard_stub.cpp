#include "burnint.h"
#include "pgm2_memcard.h"

void pgm2MemcardInit(UINT32) {}
void pgm2MemcardExit() {}
void pgm2MemcardReset() {}
void pgm2MemcardSetPresent(INT32) {}
INT32 pgm2MemcardPresent() { return 0; }
void pgm2MemcardSetPresentSlot(UINT32, INT32) {}
INT32 pgm2MemcardPresentSlot(UINT32) { return 0; }
INT32 pgm2MemcardAuthenticatedSlot(UINT32) { return 0; }
INT32 pgm2MemcardDirtySlot(UINT32) { return 0; }
UINT32 pgm2MemcardGetSize(UINT32) { return 0; }
void pgm2MemcardLoadDefault(const UINT8*, UINT32) {}
INT32 pgm2MemcardCreateImage(UINT8*, UINT32) { return 1; }
INT32 pgm2MemcardLoadImageSlot(UINT32, const UINT8*, UINT32) { return 1; }
INT32 pgm2MemcardSaveImageSlot(UINT32, UINT8*, UINT32) { return 1; }
void pgm2MemcardEjectSlot(UINT32) {}

UINT8 pgm2MemcardRead8(UINT32) { return 0xFF; }
void pgm2MemcardWrite8(UINT32, UINT8) {}
UINT8 pgm2MemcardReadProt(UINT32) { return 0xFF; }
void pgm2MemcardWriteProt(UINT32, UINT8) {}
UINT8 pgm2MemcardReadSec(UINT32) { return 0xFF; }
void pgm2MemcardWriteSec(UINT32, UINT8) {}
void pgm2MemcardAuth(UINT8, UINT8, UINT8) {}

UINT8 pgm2MemcardRead8Slot(UINT32, UINT32) { return 0xFF; }
void pgm2MemcardWrite8Slot(UINT32, UINT32, UINT8) {}
UINT8 pgm2MemcardReadProtSlot(UINT32, UINT32) { return 0xFF; }
void pgm2MemcardWriteProtSlot(UINT32, UINT32, UINT8) {}
UINT8 pgm2MemcardReadSecSlot(UINT32, UINT32) { return 0xFF; }
void pgm2MemcardWriteSecSlot(UINT32, UINT32, UINT8) {}
void pgm2MemcardAuthSlot(UINT32, UINT8, UINT8, UINT8) {}

void pgm2MemcardScan(INT32) {}
