#include "pgm2_memcard.h"
#include "pgm2.h"

#include <string.h>

static const UINT32 PGM2_COMPAT_CARD_SIZE = 0x108;
static const UINT32 PGM2_COMPAT_CARD_DATA_SIZE = 0x100;

static UINT8 gPgm2CompatAuthenticated[4] = { 0, 0, 0, 0 };
static UINT8 gPgm2CompatDirty[4] = { 0, 0, 0, 0 };
static UINT8 gPgm2CompatTemplate[PGM2_COMPAT_CARD_SIZE];
static bool  gPgm2CompatTemplateValid = false;

static inline UINT32 pgm2CompatSlot(UINT32 slot)
{
	return slot & 3;
}

static inline UINT8* pgm2CompatCard(UINT32 slot)
{
	return Pgm2Cards[pgm2CompatSlot(slot)];
}

static bool pgm2CompatEnsureCard(UINT32 slot)
{
	slot = pgm2CompatSlot(slot);

	if (Pgm2Cards[slot]) {
		return true;
	}

	Pgm2Cards[slot] = (UINT8*)BurnMalloc(PGM2_COMPAT_CARD_SIZE);
	if (!Pgm2Cards[slot]) {
		return false;
	}

	memset(Pgm2Cards[slot], 0xFF, PGM2_COMPAT_CARD_SIZE);
	Pgm2Cards[slot][PGM2_COMPAT_CARD_DATA_SIZE + 4] = 0x07;

	return true;
}

static void pgm2CompatFillBlank(UINT8* data, UINT32 len)
{
	if (!data || len < PGM2_COMPAT_CARD_SIZE) {
		return;
	}

	memset(data, 0xFF, PGM2_COMPAT_CARD_SIZE);
	data[PGM2_COMPAT_CARD_DATA_SIZE + 4] = 0x07;
}

void pgm2MemcardInit(UINT32)
{
	memset(gPgm2CompatAuthenticated, 0, sizeof(gPgm2CompatAuthenticated));
	memset(gPgm2CompatDirty, 0, sizeof(gPgm2CompatDirty));
	memset(gPgm2CompatTemplate, 0xFF, sizeof(gPgm2CompatTemplate));
	gPgm2CompatTemplate[PGM2_COMPAT_CARD_DATA_SIZE + 4] = 0x07;
	gPgm2CompatTemplateValid = false;
}

void pgm2MemcardExit()
{
	memset(gPgm2CompatAuthenticated, 0, sizeof(gPgm2CompatAuthenticated));
	memset(gPgm2CompatDirty, 0, sizeof(gPgm2CompatDirty));
	gPgm2CompatTemplateValid = false;
}

void pgm2MemcardReset()
{
	memset(gPgm2CompatAuthenticated, 0, sizeof(gPgm2CompatAuthenticated));
}

void pgm2MemcardSetPresent(INT32 present)
{
	for (UINT32 slot = 0; slot < 4; slot++) {
		pgm2MemcardSetPresentSlot(slot, present);
	}
}

INT32 pgm2MemcardPresent()
{
	return pgm2MemcardPresentSlot(0);
}

void pgm2MemcardSetPresentSlot(UINT32 slot, INT32 present)
{
	slot = pgm2CompatSlot(slot);
	Pgm2CardInserted[slot] = present ? true : false;
	if (!Pgm2CardInserted[slot]) {
		gPgm2CompatAuthenticated[slot] = 0;
	}
}

INT32 pgm2MemcardPresentSlot(UINT32 slot)
{
	slot = pgm2CompatSlot(slot);
	return (pgm2CompatCard(slot) != NULL && Pgm2CardInserted[slot]) ? 1 : 0;
}

INT32 pgm2MemcardAuthenticatedSlot(UINT32 slot)
{
	return gPgm2CompatAuthenticated[pgm2CompatSlot(slot)] ? 1 : 0;
}

INT32 pgm2MemcardDirtySlot(UINT32 slot)
{
	return gPgm2CompatDirty[pgm2CompatSlot(slot)] ? 1 : 0;
}

UINT32 pgm2MemcardGetSize(UINT32)
{
	return PGM2_COMPAT_CARD_SIZE;
}

void pgm2MemcardLoadDefault(const UINT8* data, UINT32 len)
{
	if (!data || len < PGM2_COMPAT_CARD_SIZE) {
		return;
	}

	memcpy(gPgm2CompatTemplate, data, PGM2_COMPAT_CARD_SIZE);
	gPgm2CompatTemplateValid = true;
}

INT32 pgm2MemcardCreateImage(UINT8* data, UINT32 len)
{
	if (!data || len < PGM2_COMPAT_CARD_SIZE) {
		return 1;
	}

	if (pgm2GetCardRomTemplate(data, len) == PGM2_COMPAT_CARD_SIZE) {
		return 0;
	}

	if (gPgm2CompatTemplateValid) {
		memcpy(data, gPgm2CompatTemplate, PGM2_COMPAT_CARD_SIZE);
		return 0;
	}

	pgm2CompatFillBlank(data, len);
	return 0;
}

INT32 pgm2MemcardLoadImageSlot(UINT32 slot, const UINT8* data, UINT32 len)
{
	slot = pgm2CompatSlot(slot);
	if (!data || len < PGM2_COMPAT_CARD_SIZE || !pgm2CompatEnsureCard(slot)) {
		return 1;
	}

	memcpy(Pgm2Cards[slot], data, PGM2_COMPAT_CARD_SIZE);
	Pgm2CardInserted[slot] = true;
	gPgm2CompatAuthenticated[slot] = 0;
	gPgm2CompatDirty[slot] = 0;

	return 0;
}

INT32 pgm2MemcardSaveImageSlot(UINT32 slot, UINT8* data, UINT32 len)
{
	slot = pgm2CompatSlot(slot);
	if (!data || len < PGM2_COMPAT_CARD_SIZE || !pgm2CompatCard(slot)) {
		return 1;
	}

	memcpy(data, Pgm2Cards[slot], PGM2_COMPAT_CARD_SIZE);
	gPgm2CompatDirty[slot] = 0;
	return 0;
}

void pgm2MemcardEjectSlot(UINT32 slot)
{
	slot = pgm2CompatSlot(slot);
	Pgm2CardInserted[slot] = false;
	gPgm2CompatAuthenticated[slot] = 0;
	gPgm2CompatDirty[slot] = 0;
}

UINT8 pgm2MemcardRead8(UINT32 addr)
{
	return pgm2MemcardRead8Slot(0, addr);
}

void pgm2MemcardWrite8(UINT32 addr, UINT8 data)
{
	pgm2MemcardWrite8Slot(0, addr, data);
}

UINT8 pgm2MemcardReadProt(UINT32 addr)
{
	return pgm2MemcardReadProtSlot(0, addr);
}

void pgm2MemcardWriteProt(UINT32 addr, UINT8 data)
{
	pgm2MemcardWriteProtSlot(0, addr, data);
}

UINT8 pgm2MemcardReadSec(UINT32 addr)
{
	return pgm2MemcardReadSecSlot(0, addr);
}

void pgm2MemcardWriteSec(UINT32 addr, UINT8 data)
{
	pgm2MemcardWriteSecSlot(0, addr, data);
}

void pgm2MemcardAuth(UINT8 p1, UINT8 p2, UINT8 p3)
{
	pgm2MemcardAuthSlot(0, p1, p2, p3);
}

UINT8 pgm2MemcardRead8Slot(UINT32 slot, UINT32 addr)
{
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= PGM2_COMPAT_CARD_DATA_SIZE) {
		return 0xFF;
	}

	return card[addr];
}

void pgm2MemcardWrite8Slot(UINT32 slot, UINT32 addr, UINT8 data)
{
	slot = pgm2CompatSlot(slot);
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= PGM2_COMPAT_CARD_DATA_SIZE || !gPgm2CompatAuthenticated[slot]) {
		return;
	}

	UINT8 prot_byte = card[PGM2_COMPAT_CARD_DATA_SIZE + (addr >> 3)];
	if (!(prot_byte & (1 << (addr & 7)))) {
		return;
	}

	card[addr] = data;
	gPgm2CompatDirty[slot] = 1;
}

UINT8 pgm2MemcardReadProtSlot(UINT32 slot, UINT32 addr)
{
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= 4) {
		return 0xFF;
	}

	return card[PGM2_COMPAT_CARD_DATA_SIZE + addr];
}

void pgm2MemcardWriteProtSlot(UINT32 slot, UINT32 addr, UINT8 data)
{
	slot = pgm2CompatSlot(slot);
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= 4 || !gPgm2CompatAuthenticated[slot]) {
		return;
	}

	card[PGM2_COMPAT_CARD_DATA_SIZE + addr] &= data;
	gPgm2CompatDirty[slot] = 1;
}

UINT8 pgm2MemcardReadSecSlot(UINT32 slot, UINT32 addr)
{
	slot = pgm2CompatSlot(slot);
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= 4 || !gPgm2CompatAuthenticated[slot]) {
		return 0xFF;
	}

	return card[PGM2_COMPAT_CARD_DATA_SIZE + 4 + addr];
}

void pgm2MemcardWriteSecSlot(UINT32 slot, UINT32 addr, UINT8 data)
{
	slot = pgm2CompatSlot(slot);
	UINT8* card = pgm2CompatCard(slot);
	if (!card || addr >= 4 || !gPgm2CompatAuthenticated[slot]) {
		return;
	}

	card[PGM2_COMPAT_CARD_DATA_SIZE + 4 + addr] = data;
	gPgm2CompatDirty[slot] = 1;
}

void pgm2MemcardAuthSlot(UINT32 slot, UINT8 p1, UINT8 p2, UINT8 p3)
{
	slot = pgm2CompatSlot(slot);
	UINT8* card = pgm2CompatCard(slot);
	if (!card) {
		gPgm2CompatAuthenticated[slot] = 0;
		return;
	}

	UINT8* sec = card + PGM2_COMPAT_CARD_DATA_SIZE + 4;

	if (sec[1] == p1 && sec[2] == p2 && sec[3] == p3) {
		if (sec[0] & 7) {
			gPgm2CompatAuthenticated[slot] = 1;
			sec[0] = 7;
		}
	} else {
		gPgm2CompatAuthenticated[slot] = 0;
		sec[0] >>= 1;
	}
}

void pgm2MemcardScan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(gPgm2CompatAuthenticated);
		SCAN_VAR(gPgm2CompatDirty);
	}
}
