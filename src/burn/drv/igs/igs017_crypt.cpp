#include "bitswap.h"
#include "igs017.h"

void igs017_crypt_program_rom(UINT8 *rom, INT32 len, UINT8 mask, INT32 a7, INT32 a6, INT32 a5, INT32 a4, INT32 a3, INT32 a2, INT32 a1, INT32 a0)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) return;

	for (INT32 i = 0; i < len; i++) {
		if (i & 0x2000) {
			if ((i & mask) == mask) rom[i] ^= 0x01;
		} else if (i & 0x0100) {
			if ((i & mask) == mask) rom[i] ^= 0x01;
		} else if (i & 0x0080) {
			if ((i & mask) == mask) rom[i] ^= 0x01;
		} else {
			if ((i & mask) != mask) rom[i] ^= 0x01;
		}
	}

	memcpy(tmp, rom, len);

	for (INT32 i = 0; i < len; i++) {
		INT32 addr = (i & ~0xff) | BITSWAP08(i, a7, a6, a5, a4, a3, a2, a1, a0);
		rom[i] = tmp[addr];
	}

	BurnFree(tmp);
}

void igs017_crypt_sdmg2754ca(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (i & (0x20000 / 2)) x ^= 0x0200; else if (!(i & (0x400 / 2))) x ^= 0x0200;
		if (i & (0x20000 / 2)) x ^= 0x1000;
		r[i] = x;
	}
}

void igs017_crypt_sdmg2754cb(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (i & (0x20000 / 2)) x ^= 0x0200; else if (!(i & (0x200 / 2))) x ^= 0x0200;
		if (i & (0x20000 / 2)) x ^= 0x1000;
		r[i] = x;
	}
}

void igs017_crypt_sdmg2(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (!(i & (0x20000 / 2)) && (i & (0x400 / 2))) x ^= 0x0200;
		if (i & (0x20000 / 2)) x ^= 0x1000;
		r[i] = x;
	}
}

void igs017_crypt_sdmg2p(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if (((i & (0x4320 / 2)) == (0x0000 / 2)) || ((i & (0x4322 / 2)) == (0x0020 / 2)) || ((i & (0x0122 / 2)) == (0x0122 / 2)) || ((i & (0x0222 / 2)) == (0x0222 / 2)) || ((i & (0x4322 / 2)) == (0x4022 / 2))) x ^= 0x0001;
		if ((i & (0x4000 / 2)) || (i & (0x0200 / 2)) || ((i & (0x4b68 / 2)) == (0x0048 / 2)) || ((i & (0x4b40 / 2)) == (0x0840 / 2))) x ^= 0x0040;
		if (((i & (0x60000 / 2)) == (0x00000 / 2)) || ((i & (0x60000 / 2)) == (0x60000 / 2))) x ^= 0x2000;
		r[i] = x;
	}
}

void igs017_crypt_mgdha(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) {
			if ((i & (0x300 / 2)) || (i & (0x4000 / 2))) x ^= 0x0001;
		} else {
			if (!(i & (0x300 / 2)) && !(i & (0x4000 / 2))) x ^= 0x0001;
		}
		if (i & (0x60000 / 2)) x ^= 0x0100;
		if ((i & (0x1000 / 2)) || ((i & (0x4000 / 2)) && (i & (0x40 / 2)) && (i & (0x80 / 2))) || ((i & (0x2000 / 2)) && (i & (0x400 / 2)))) x ^= 0x0800;
		r[i] = x;
	}
}

void igs017_crypt_mgcs(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if ((i & (0x2000 / 2)) || !(i & (0x80 / 2))) {
			if ((i & (0x100 / 2)) && (!(i & (0x20 / 2)) || (i & (0x400 / 2)))) x ^= 0x0100;
		} else {
			x ^= 0x0100;
		}
		r[i] = x;
	}
}

void igs017_crypt_mgcsa(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (!(i & (0x1000 / 2)) && !(i & (0x100 / 2))) {
			if (!(i & (0x20 / 2))) x ^= 0x0100;
		} else if (i & (0x1000 / 2)) {
			if (!(i & (0x100 / 2)) || ((i & (0x20 / 2)) && !(i & (0x400 / 2)))) x ^= 0x0100;
		}
		r[i] = x;
	}
}

void igs017_crypt_mgcsb(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if ((i & (0x50100 / 2)) == (0x10100 / 2)) x ^= 0x0200;
		if ((i & (0x50300 / 2)) == (0x00100 / 2)) x ^= 0x0200;
		if ((i & (0x52300 / 2)) == (0x52300 / 2)) x ^= 0x0200;
		r[i] = x;
	}
}

void igs017_crypt_lhzb2(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (!(i & (0x1000 / 2))) {
			if (i & (0x2000 / 2)) {
				if (i & (0x8000 / 2)) {
					if (!(i & (0x100 / 2))) {
						if ((i & (0x200 / 2)) && !(i & (0x40 / 2))) x ^= 0x2000;
						if (!(i & (0x200 / 2))) x ^= 0x2000;
					}
				} else {
					if (!(i & (0x100 / 2))) x ^= 0x2000;
				}
			} else {
				if (i & (0x8000 / 2)) {
					if ((i & (0x200 / 2)) && !(i & (0x40 / 2))) x ^= 0x2000;
					if (!(i & (0x200 / 2))) x ^= 0x2000;
				} else {
					x ^= 0x2000;
				}
			}
		}
		r[i] = x;
	}
}

void igs017_crypt_lhzb2a(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (i & (0x4000 / 2)) {
			if (i & (0x8000 / 2)) {
				if (i & (0x2000 / 2)) {
					if ((i & (0x200 / 2)) && (!(i & (0x40 / 2)) || (i & (0x800 / 2)))) x ^= 0x0020;
				}
			} else {
				if (!(i & (0x40 / 2)) || (i & (0x800 / 2))) x ^= 0x0020;
			}
		}
		r[i] = x;
	}
}

void igs017_crypt_lhzb2b(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (i & (0x8000 / 2)) {
			if (i & (0x2000 / 2)) {
				if (!(i & (0x800 / 2))) {
					if (!(i & (0x200 / 2)) && (i & (0x100 / 2)) && (i & (0x40 / 2))) x ^= 0x0400;
				} else {
					if ((i & (0x100 / 2)) && (i & (0x40 / 2))) x ^= 0x0400;
				}
			}
		} else {
			if (!(i & (0x800 / 2))) {
				if (!(i & (0x200 / 2)) && (i & (0x40 / 2))) x ^= 0x0400;
			} else {
				if (i & (0x40 / 2)) x ^= 0x0400;
			}
		}
		r[i] = x;
	}
}

void igs017_crypt_lhzb2c(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];
		if ((i & (0x20 / 2)) && (i & (0x02 / 2))) x ^= 0x0001;
		if (!(i & (0x4000 / 2)) && !(i & (0x300 / 2))) x ^= 0x0001;
		if (i & (0x4000 / 2)) {
			if (i & (0x8000 / 2)) {
				if (!(i & (0x2000 / 2))) {
					if ((i & (0x200 / 2)) && (!(i & (0x40 / 2)) || (i & (0x800 / 2)))) x ^= 0x0020;
				}
			} else {
				if (!(i & (0x40 / 2)) || (i & (0x800 / 2))) x ^= 0x0020;
			}
		}
		r[i] = x;
	}
}

void igs017_crypt_slqz2(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];

		// bit 0 xor layer

		if (i & 0x20/2)
		{
			if (i & 0x02/2)
			{
				x ^= 0x0001;
			}
		}

		if (!(i & 0x4000/2))
		{
			if (!(i & 0x300/2))
			{
				x ^= 0x0001;
			}
		}

		// bit 14 xor layer

		if (i & 0x1000/2)
		{
			if (i & 0x800/2)
			{
				x ^= 0x4000;
			}
			else
			{
				if (i & 0x200/2)
				{
					if (!(i & 0x100/2))
					{
						if (i & 0x40/2)
						{
							x ^= 0x4000;
						}
					}
				}
				else
				{
					x ^= 0x4000;
				}
			}
		}
		else
		{
			if (i & 0x800/2)
			{
				x ^= 0x4000;
			}
			else
			{
				if (!(i & 0x100/2))
				{
					if (i & 0x40/2)
					{
						x ^= 0x4000;
					}
				}
			}
		}

		r[i] = x;
	}
}

void igs017_crypt_slqz2b(UINT8 *rom, INT32 len)
{
	UINT16 *r = (UINT16*)rom;
	for (INT32 i = 0; i < len / 2; i++) {
		UINT16 x = r[i];

		// bit 0 xor layer

		if (i & 0x20/2)
		{
			if (i & 0x02/2)
			{
				x ^= 0x0001;
			}
		}

		if (!(i & 0x4000/2))
		{
			if (!(i & 0x300/2))
			{
				x ^= 0x0001;
			}
		}

		// bit 14 xor layer

		if (!(i & 0x800/2))
		{
			if (i & 0x1000/2)
			{
				if (i & 0x200/2)
				{
					if (!(i & 0x100/2))
						{
							if (!(i & 0x40/2))
							{
								x ^= 0x4000;
							}
						}
					else
					{
						x ^= 0x4000;
					}
				}
			}
			else
			{
				if (i & 0x100/2)
				{
					x ^= 0x4000;
				}
				else
				{
					if (!(i & 0x40/2))
					{
						x ^= 0x4000;
					}
				}
			}
		}

		r[i] = x;
	}
}
