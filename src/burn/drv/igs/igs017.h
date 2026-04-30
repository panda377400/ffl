#ifndef IGS017_H
#define IGS017_H

#include "burnint.h"
#include "bitswap.h"

#define IGS017_WORD_SWAP (1 << 16)

void igs017_crypt_program_rom(UINT8 *rom, INT32 len, UINT8 mask, INT32 a7, INT32 a6, INT32 a5, INT32 a4, INT32 a3, INT32 a2, INT32 a1, INT32 a0);

void igs017_crypt_sdmg2754ca(UINT8 *rom, INT32 len);
void igs017_crypt_sdmg2754cb(UINT8 *rom, INT32 len);
void igs017_crypt_sdmg2(UINT8 *rom, INT32 len);
void igs017_crypt_sdmg2p(UINT8 *rom, INT32 len);
void igs017_crypt_mgdha(UINT8 *rom, INT32 len);
void igs017_crypt_mgcs(UINT8 *rom, INT32 len);
void igs017_crypt_mgcsa(UINT8 *rom, INT32 len);
void igs017_crypt_mgcsb(UINT8 *rom, INT32 len);
void igs017_crypt_lhzb2(UINT8 *rom, INT32 len);
void igs017_crypt_lhzb2a(UINT8 *rom, INT32 len);
void igs017_crypt_lhzb2b(UINT8 *rom, INT32 len);
void igs017_crypt_lhzb2c(UINT8 *rom, INT32 len);
void igs017_crypt_slqz2(UINT8 *rom, INT32 len);
void igs017_crypt_slqz2b(UINT8 *rom, INT32 len);

static inline UINT16 igs017_string_bitswap_step(UINT16 value, UINT16 string_word, UINT8 offset, UINT8 data)
{
	UINT16 x = value;
	UINT16 result = 0;

	result |= (BIT( x,  0) ^ BIT(string_word,  0)) << 1;
	result |= (BIT(~x,  1) ^ BIT(string_word,  1)) << 2;
	result |= (BIT(~x,  2) ^ BIT(string_word,  2)) << 3;
	result |= (BIT(~x, 13) ^ BIT(          x,  3)) << 4;
	result |= (BIT(~x,  4) ^ BIT(string_word,  4)) << 5;
	result |= (BIT( x,  5) ^ BIT(string_word,  5)) << 6;
	result |= (BIT(~x,  6) ^ BIT(string_word,  6)) << 7;
	result |= (BIT(~x,  7) ^ BIT(string_word,  7)) << 8;
	result |= (BIT(~x,  8) ^ BIT(string_word,  8)) << 9;
	result |= (BIT( x,  9) ^ BIT(string_word,  9)) << 10;
	result |= (BIT(~x, 10) ^ BIT(          x,  3)) << 11;
	result |= (BIT( x, 11) ^ BIT(string_word, 11)) << 12;
	result |= (BIT(~x, 12) ^ BIT(string_word, 12)) << 13;
	result |= (BIT( x, 13) ^ BIT(string_word, 13)) << 14;
	result |= (BIT( x, 14) ^ BIT(string_word, 14)) << 15;

	UINT16 bit0 = BIT(~x, 15) ^ BIT(x, 7);
	UINT16 xor0 = BIT(data, offset & 7);
	result |= bit0 ^ xor0;

	return result;
}

#endif
