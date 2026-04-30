#include "burner.h"
#include "retro_common.h"
#include "retro_dirent.h"
#include <cctype>
#include <time.h>
#include <file/file_path.h>

#ifndef UINT32_MAX
#define UINT32_MAX	(UINT32)4294967295U
#endif

// d_cps1.cpp
#define CPS1_68K_PROGRAM_BYTESWAP			1
#define CPS1_68K_PROGRAM_NO_BYTESWAP		2
#define CPS1_Z80_PROGRAM					3
#define CPS1_TILES							4
#define CPS1_OKIM6295_SAMPLES				5
#define CPS1_QSOUND_SAMPLES					6
#define CPS1_PIC							7
#define CPS1_EXTRA_TILES_SF2EBBL_400000		8
#define CPS1_EXTRA_TILES_400000				9
#define CPS1_EXTRA_TILES_SF2KORYU_400000	10
#define CPS1_EXTRA_TILES_SF2B_400000		11
#define CPS1_EXTRA_TILES_SF2MKOT_400000		12

// d_cps2.cpp
#define CPS2_PRG_68K						1
#define CPS2_PRG_68K_SIMM					2
#define CPS2_PRG_68K_XOR_TABLE				3
#define CPS2_GFX							5
#define CPS2_GFX_SIMM						6
#define CPS2_GFX_SPLIT4						7
#define CPS2_GFX_SPLIT8						8
#define CPS2_GFX_19XXJ						9
#define CPS2_PRG_Z80						10
#define CPS2_QSND							12
#define CPS2_QSND_SIMM						13
#define CPS2_QSND_SIMM_BYTESWAP				14
#define CPS2_ENCRYPTION_KEY					15

// gal.h
#define GAL_ROM_Z80_PROG1				1
#define GAL_ROM_Z80_PROG2				2
#define GAL_ROM_Z80_PROG3				3
#define GAL_ROM_TILES_SHARED			4
#define GAL_ROM_TILES_CHARS				5
#define GAL_ROM_TILES_SPRITES			6
#define GAL_ROM_PROM					7
#define GAL_ROM_S2650_PROG1				8

// megadrive.h
#define SEGA_MD_ROM_LOAD_NORMAL								0x10
#define SEGA_MD_ROM_LOAD16_WORD_SWAP						0x20
#define SEGA_MD_ROM_LOAD16_BYTE								0x30
#define SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000	0x40
#define SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000		0x50
#define SEGA_MD_ROM_OFFS_000000								0x01
#define SEGA_MD_ROM_OFFS_000001								0x02
#define SEGA_MD_ROM_OFFS_020000								0x03
#define SEGA_MD_ROM_OFFS_080000								0x04
#define SEGA_MD_ROM_OFFS_100000								0x05
#define SEGA_MD_ROM_OFFS_100001								0x06
#define SEGA_MD_ROM_OFFS_200000								0x07
#define SEGA_MD_ROM_OFFS_300000								0x08
#define SEGA_MD_ROM_RELOAD_200000_200000					0x09
#define SEGA_MD_ROM_RELOAD_100000_300000					0x0a
#define SEGA_MD_ARCADE_SUNMIXING							(0x4000)

// sys16.h
#define SYS16_ROM_PROG_FLAT									25
#define SYS16_ROM_PROG										1
#define SYS16_ROM_TILES										2
#define SYS16_ROM_SPRITES									3
#define SYS16_ROM_Z80PROG									4
#define SYS16_ROM_KEY										5
#define SYS16_ROM_7751PROG									6
#define SYS16_ROM_7751DATA									7
#define SYS16_ROM_UPD7759DATA								8
#define SYS16_ROM_PROG2										9
#define SYS16_ROM_ROAD										10
#define SYS16_ROM_PCMDATA									11
#define SYS16_ROM_Z80PROG2									12
#define SYS16_ROM_Z80PROG3									13
#define SYS16_ROM_Z80PROG4									14
#define SYS16_ROM_PCM2DATA									15
#define SYS16_ROM_PROM 										16
#define SYS16_ROM_PROG3										17
#define SYS16_ROM_SPRITES2									18
#define SYS16_ROM_RF5C68DATA								19
#define SYS16_ROM_I8751										20
#define SYS16_ROM_MSM6295									21
#define SYS16_ROM_TILES_20000								22

// taito.h
#define TAITO_68KROM1										1
#define TAITO_68KROM1_BYTESWAP								2
#define TAITO_68KROM1_BYTESWAP_JUMPING						3
#define TAITO_68KROM1_BYTESWAP32							4
#define TAITO_68KROM2										5
#define TAITO_68KROM2_BYTESWAP								6
#define TAITO_68KROM3										7
#define TAITO_68KROM3_BYTESWAP								8
#define TAITO_Z80ROM1										9
#define TAITO_Z80ROM2										10
#define TAITO_CHARS											11
#define TAITO_CHARS_BYTESWAP								12
#define TAITO_CHARSB										13
#define TAITO_CHARSB_BYTESWAP								14
#define TAITO_SPRITESA										15
#define TAITO_SPRITESA_BYTESWAP								16
#define TAITO_SPRITESA_BYTESWAP32							17
#define TAITO_SPRITESA_TOPSPEED								18
#define TAITO_SPRITESB										19
#define TAITO_SPRITESB_BYTESWAP								20
#define TAITO_SPRITESB_BYTESWAP32							21
#define TAITO_ROAD											22
#define TAITO_SPRITEMAP										23
#define TAITO_YM2610A										24
#define TAITO_YM2610B										25
#define TAITO_MSM5205										26
#define TAITO_MSM5205_BYTESWAP								27
#define TAITO_CHARS_PIVOT									28
#define TAITO_MSM6295										29
#define TAITO_ES5505										30
#define TAITO_ES5505_BYTESWAP								31
#define TAITO_DEFAULT_EEPROM								32
#define TAITO_CHARS_BYTESWAP32								33
#define TAITO_CCHIP_BIOS									34
#define TAITO_CCHIP_EEPROM									35

static TCHAR szRomset[MAX_PATH] = _T("");
static struct RomDataInfo RDI = { 0 };
RomDataInfo* pRDI = &RDI;

static void CopyTrimmedToken(char* dst, size_t dstSize, const char* src)
{
	if (dst == NULL || dstSize == 0) return;

	dst[0] = '\0';
	if (src == NULL) return;

	const unsigned char* begin = (const unsigned char*)src;
	while (*begin && isspace(*begin)) begin++;

	const unsigned char* end = begin + strlen((const char*)begin);
	while (end > begin) {
		unsigned char c = *(end - 1);
		if (!isspace(c) && c != '/' && c != '\\') break;
		end--;
	}

	size_t len = (size_t)(end - begin);
	if (len >= dstSize) len = dstSize - 1;

	if (len > 0) {
		memcpy(dst, begin, len);
	}
	dst[len] = '\0';
}

static TCHAR* SkipUtfBom(TCHAR* text)
{
	if (text == NULL) return NULL;

#ifdef _UNICODE
	if (text[0] == 0xFEFF) {
		return text + 1;
	}
#else
	unsigned char* bytes = (unsigned char*)text;
	if (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
		return text + 3;
	}
	if (bytes[0] == 0xFF && bytes[1] == 0xFE) {
		return text + 2;
	}
#endif

	return text;
}

struct BurnRomInfo* pDataRomDesc = NULL;

TCHAR szRomdataName[MAX_PATH] = _T("");
static TCHAR szFullName[MAX_PATH] = { 0 };

std::vector<romdata_core_option> romdata_core_options;

TCHAR CoreRomDataPaths[DIRS_MAX][MAX_PATH];

// 小工具：跨平台小写（注意：对 TCHAR 的处理依赖平台环境）
void StringToLower(TCHAR* str) {
    if (!str) return;
    for (TCHAR* p = str; *p; ++p) {
        *p = (TCHAR)tolower((unsigned char)*p);
    }
}

INT32 CoreRomDataPathsLoad()
{
	TCHAR szConfig[MAX_PATH] = { 0 }, szLine[1024] = { 0 };
	FILE* h = NULL;

#ifdef _UNICODE
	setlocale(LC_ALL, "");
#endif

	for (INT32 i = 0; i < DIRS_MAX; i++)
		memset(CoreRomDataPaths[i], 0, MAX_PATH * sizeof(TCHAR));

	    // [优化] — 保证 CoreRomDataPaths[0] 永远是默认路径（兼容原逻辑）
	{
		char* p = find_last_slash(szAppRomdatasPath);				// g_system_dir/fbneo/romdata/
		if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';				// 去掉末尾斜杠
		strcpy(CoreRomDataPaths[0], szAppRomdatasPath);				// CoreRomDataPaths[0] = g_system_dir/fbneo/romdata
	}

	snprintf(
		szConfig, MAX_PATH - 1, "%sromdata_path.opt",
		szAppPathDefPath
	);															// g_system_dir/fbneo/path/romdata_path.opt

	if (NULL == (h = _tfopen(szConfig, _T("rt"))))
	{
		memset(szConfig, 0, MAX_PATH * sizeof(TCHAR));
		snprintf(
			szConfig, MAX_PATH - 1, "%s%cromdata_path.opt",
			g_rom_dir, PATH_DEFAULT_SLASH_C()
		);														// g_rom_dir/romdata_path.opt

		if (NULL == (h = fopen(szConfig, "rt"))) {
			log_cb(RETRO_LOG_WARN, "[ROMDATA]\n");
			return 0;											// 只有 CoreRomDataPaths[0]
		}
	}

	// Go through each line of the config file
	while (_fgetts(szLine, 1024, h)) {
		int nLen = _tcslen(szLine);

		// Get rid of the linefeed at the end
		if (nLen > 0 && (szLine[nLen - 1] == 10 || szLine[nLen - 1] == 13)) {
			szLine[nLen - 1] = 0;
			nLen--;
			if (nLen > 0 && (szLine[nLen - 1] == 10 || szLine[nLen - 1] == 13)) {
				szLine[nLen - 1] = 0;
				nLen--;
			}
		}

#define STR(x) { TCHAR* szValue = LabelCheck(szLine,_T(#x) _T(" "));	\
  if (szValue) _tcscpy(x,szValue); }

//		STR(CoreRomDataPaths[0]);								// g_system_dir/fbneo/romdata
		STR(CoreRomDataPaths[1]);
		STR(CoreRomDataPaths[2]);
		STR(CoreRomDataPaths[3]);
		STR(CoreRomDataPaths[4]);
		STR(CoreRomDataPaths[5]);
		STR(CoreRomDataPaths[6]);
		STR(CoreRomDataPaths[7]);
		STR(CoreRomDataPaths[8]);
		STR(CoreRomDataPaths[9]);
		STR(CoreRomDataPaths[10]);
		STR(CoreRomDataPaths[11]);
		STR(CoreRomDataPaths[12]);
		STR(CoreRomDataPaths[13]);
		STR(CoreRomDataPaths[14]);
		STR(CoreRomDataPaths[15]);
		STR(CoreRomDataPaths[16]);
		STR(CoreRomDataPaths[17]);
		STR(CoreRomDataPaths[18]);
		STR(CoreRomDataPaths[19]);
#undef STR
	}

	fclose(h);
	return 0;													// There may be more
}

static INT32 IsUTF8Text(const void* pBuffer, long size)
{
	INT32 nCode = 0;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end   = (unsigned char*)pBuffer + size;

	while (start < end) {
		if (*start < 0x80) {        // (10000000) ASCII
			if (0 == nCode) nCode = 1;

			start++;
		} else if (*start < 0xc0) { // (11000000) Invalid UTF-8
			return 0;
		} else if (*start < 0xe0) { // (11100000) 2-byte UTF-8
			if (nCode < 2) nCode = 2;
			if (start >= end - 1) break;
			if (0x80 != (start[1] & 0xc0)) return 0;

			start += 2;
		} else if (*start < 0xf0) { // (11110000) 3-byte UTF-8
			if (nCode < 3) nCode = 3;
			if (start >= end - 2) break;
			if ((0x80 != (start[1] & 0xc0)) || (0x80 != (start[2] & 0xc0))) return 0;

			start += 3;
		} else {
			return 0;
		}
	}

	return nCode;
}

// [优化] 更稳健的 IsDatUTF8BOM：保证文件关闭并处理 malloc 失败
static INT32 IsDatUTF8BOM()
{
	FILE* fp = _tfopen(szRomdataName, _T("rb"));
	if (NULL == fp) return -1;

	// get dat size
	fseek(fp, 0, SEEK_END);
	INT32 nDatSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	INT32 nRet = 0;
	char* pszTest = (char*)malloc((size_t)nDatSize + 1);

	if (NULL != pszTest) {
		memset(pszTest, 0, nDatSize + 1);
		if (fread(pszTest, (size_t)nDatSize, 1, fp) == 1) {
			nRet = IsUTF8Text(pszTest, nDatSize);
		} else {
			nRet = 0;
		}
		free(pszTest);
		pszTest = NULL;
	} else {
		// [优化] 内存分配失败，不要忘记关闭 fp
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return nRet;
}

#define DELIM_TOKENS_NAME	_T(" \t\r\n,%:|{}")
#define _strqtoken	strqtoken
#define _tcscmp		strcmp
#define _stscanf	sscanf

// 兼容性的大小写不敏感比较（TCHAR 版）
static int _tcsicmp_insensitive(const TCHAR* str1, const TCHAR* str2) {
    while (*str1 && *str2) {
        int c1 = tolower((unsigned char)*str1++);
        int c2 = tolower((unsigned char)*str2++);
        if (c1 != c2) return c1 - c2;
    }
    return tolower((unsigned char)*str1) - tolower((unsigned char)*str2);
}

// C 风格的 strcasecmp 兼容函数（char*）
static int strcasecmp_cross(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1++);
        int c2 = tolower((unsigned char)*s2++);
        if (c1 != c2) return c1 - c2;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

// 跨平台CPS1类型获取函数
static UINT32 GetPlatformAgnosticCps1Type(const char* drvName, const char* romName,
                                         UINT32 len, UINT32 crc, const char* section) 
{
    // 特殊CRC覆盖 - 确保三国志2和名将世界版正确运行
    static const struct { UINT32 crc; UINT32 type; } crcOverrides[] = {
        {0x8B31FD6E, CPS1_68K_PROGRAM_NO_BYTESWAP}, // 三国志2
        {0x6AFEFCA4, CPS1_68K_PROGRAM_NO_BYTESWAP}, // 名将世界版
        {0x00000000, 0} // 结束标记
    };
    
    for (int i = 0; crcOverrides[i].crc != 0; i++) {
        if (crc == crcOverrides[i].crc) {
            return crcOverrides[i].type | BRF_PRG | BRF_ESS;
        }
    }

    // 程序段处理逻辑
    if (strcasecmp_cross(section, "Program") == 0) {
        /* 字节交换规则:
           1. 0x200000长度必定不交换
           2. 文件名含"nobyteswap"不交换
           3. 默认需要交换 */
        if (len == 0x200000) {
            return CPS1_68K_PROGRAM_NO_BYTESWAP | BRF_PRG | BRF_ESS;
        }
        
        if (romName && (strstr(romName, "nobyteswap") || strstr(romName, "NOBYTESWAP"))) {
            return CPS1_68K_PROGRAM_NO_BYTESWAP | BRF_PRG | BRF_ESS;
        }
        
        return CPS1_68K_PROGRAM_BYTESWAP | BRF_PRG | BRF_ESS;
    }
    
    // 其他段类型处理
    const struct {
        const char* section;
        UINT32 defaultType;
    } sectionMap[] = {
        {"Z80",       CPS1_Z80_PROGRAM | BRF_PRG | BRF_ESS},
        {"Graphics",  CPS1_TILES | BRF_GRA},
        {"Samples",   CPS1_OKIM6295_SAMPLES | BRF_SND}, // 默认OKIM6295
        {"Pic",       CPS1_PIC | BRF_PRG}
    };

    for (size_t i = 0; i < sizeof(sectionMap)/sizeof(sectionMap[0]); i++) {
        if (strcasecmp_cross(section, sectionMap[i].section) == 0) {
            // QSound特殊处理
            if (strcasecmp_cross(section, "Samples") == 0) {
                // 文件名包含"q"或"qsound"
                if (romName && (strstr(romName, "q") || strstr(romName, "Q") || 
                                 strstr(romName, "qsound") || strstr(romName, "QSOUND"))) {
                    return CPS1_QSOUND_SAMPLES | BRF_SND;
                }
                
                // 已知使用QSound的游戏
                if (drvName && (strstr(drvName, "sf2") || strstr(drvName, "SF2") || 
                                strstr(drvName, "sfz") || strstr(drvName, "SFZ") ||
                                strstr(drvName, "wof") || strstr(drvName, "WOF") || 
                                strstr(drvName, "tk2") || strstr(drvName, "TK2") ||
                                strstr(drvName, "varth") || strstr(drvName, "VARTH"))) {
                    return CPS1_QSOUND_SAMPLES | BRF_SND;
                }
            }
            return sectionMap[i].defaultType;
        }
    }

    // 未识别段类型
    return 0;
}

#define DELIM_TOKENS_NAME	_T(" \t\r\n,%:|{}")
#define _strqtoken	strqtoken
#define _tcscmp		strcmp
#define _stscanf	sscanf

// 解析Nebula格式ROMDATA
static INT32 ParseNebulaRomData(FILE* fp)
{
    TCHAR szBuf[1024] = { 0 }; // 增大缓冲区
    TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;
    TCHAR szRomMask[30] = { 0 };
    INT32 systemType = 0; // 0=未知, 1=Neo, 2=PGM, 3=CPS1, 4=CPS2, 5=CPS3, 6=Megadrive

    // 首先尝试检测系统类型
    bool bSystemDetected = false;
    while (!bSystemDetected && !feof(fp)) {
        if (_fgetts(szBuf, sizeof(szBuf), fp) != NULL) {
            pszBuf = SkipUtfBom(szBuf);
            
            // 尝试检测System行
            char* pSystem = strstr(pszBuf, "System:");
            if (!pSystem) pSystem = strstr(pszBuf, "SYSTEM:");
            
            if (pSystem) {
                pSystem += 7; // 跳过"System:"
                while (*pSystem && isspace(*pSystem)) pSystem++; // 跳过空格
                
                if (strstr(pSystem, "CPS1")) systemType = 3;
                else if (strstr(pSystem, "neo") || strstr(pSystem, "neogeo")) systemType = 1;
                else if (strstr(pSystem, "pgm") || strstr(pSystem, "igs")) systemType = 2;
                else if (strstr(pSystem, "CPS2")) systemType = 4;
                else if (strstr(pSystem, "CPS3")) systemType = 5;
                else if (strstr(pSystem, "MEGADRIVE") || strstr(pSystem, "GENESIS") || strstr(pSystem, "SEGA")) systemType = 6;
                
                if (systemType > 0) {
                    bSystemDetected = true;
                    //log_cb(RETRO_LOG_INFO, "[ROMDATA] 🔍 检测到基板类型: %d\n", systemType);
                }
            }
        }
    }
    
    // 重置文件指针到开头
    fseek(fp, 0, SEEK_SET);

    // 主解析循环
    while (!feof(fp)) {
        if (_fgetts(szBuf, sizeof(szBuf), fp) != NULL) {
            pszBuf = SkipUtfBom(szBuf);
            
            if ((_T('#') == pszBuf[0]) || 
                ((_T('/') == pszBuf[0]) && (_T('/') == pszBuf[1])) ||
                ((_T('%') == pszBuf[0]) && (_T('%') == pszBuf[1]))) 
            {
                continue; // 仅跳过以#、//或%%开头的行
            }
            
            // ==== 增强的System行识别 ====
            char* pSystem = strstr(pszBuf, "System:");
            if (!pSystem) pSystem = strstr(pszBuf, "SYSTEM:");
            
            if (pSystem) {
                pSystem += 7; // 跳过"System:"
                
                // 移除尾部空白和换行符
                char* end = pSystem + strlen(pSystem) - 1;
                while (end > pSystem && isspace((unsigned char)*end)) end--;
                *(end + 1) = '\0';
                
                // 转换为小写比较
                char tmp[128] = {0};
                strncpy(tmp, pSystem, sizeof(tmp)-1);
                for (char* p = tmp; *p; p++) *p = tolower(*p);
                
                // 不区分大小写匹配
                if (strstr(tmp, "cps1")) systemType = 3;
                else if (strstr(tmp, "neo")) systemType = 1;
                else if (strstr(tmp, "pgm")) systemType = 2;
                else if (strstr(tmp, "cps2")) systemType = 4;
                else if (strstr(tmp, "cps3")) systemType = 5;
                else if (strstr(tmp, "megadrive") || strstr(tmp, "genesis") || strstr(tmp, "sega")) systemType = 6;
                
                if (systemType > 0) {
                    bSystemDetected = true;
                    //log_cb(RETRO_LOG_INFO, "[ROMDATA] 🔍 检测到基板类型: %d (%s)\n", systemType, pSystem);
                }
            }
            
            // ==== 原有解析逻辑 ====
            pszLabel = strqtoken(pszBuf, DELIM_TOKENS_NAME);
            if (NULL == pszLabel) continue;
            
            // 检查段标记
            UINT32 nLen = _tcslen(pszLabel);
            if ((nLen > 2) && (_T('[') == pszLabel[0]) && (_T(']') == pszLabel[nLen - 1])) {
                pszLabel++, nLen -= 2;
                if (0 == _tcsicmp_insensitive(_T("System"), pszLabel)) break;
                _tcsncpy(szRomMask, pszLabel, nLen);
                szRomMask[nLen] = _T('\0');
                //log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 🔍 发现ROM段: %s\n", szRomMask);
                continue;
            }

            // 解析元数据
            if (0 == _tcsicmp_insensitive(_T("System"), pszLabel)) {
                pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                if (NULL != pszInfo) {
                    // 创建临时缓冲区进行大小写转换
                    TCHAR szSystemLower[32] = {0};
                    _tcsncpy(szSystemLower, pszInfo, 31);
                    
                    // 转换为小写以简化比较
                    for (TCHAR* p = szSystemLower; *p; ++p) *p = tolower(*p);
                    
                    if (_tcsstr(szSystemLower, _T("neo"))) {
                        systemType = 1;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到Neo Geo基板系统\n");
                    }
                    else if (_tcsstr(szSystemLower, _T("pgm"))) {
                        systemType = 2;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到PGM基板系统\n");
                    }
                    else if (_tcsstr(szSystemLower, _T("cps1"))) {
                        systemType = 3;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到CPS1基板系统\n");
                    }
                    else if (_tcsstr(szSystemLower, _T("cps2"))) {
                        systemType = 4;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到CPS2基板系统\n");
                    }
                    else if (_tcsstr(szSystemLower, _T("cps3"))) {
                        systemType = 5;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到CPS3基板系统\n");
                    }
                    else if (_tcsstr(szSystemLower, _T("megadrive")) || 
                             _tcsstr(szSystemLower, _T("genesis")) || 
                             _tcsstr(szSystemLower, _T("sega"))) {
                        systemType = 6;
                        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到Sega Megadrive基板系统\n");
                    }
                    else {
                        log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 未知基板类型: %s\n", pszInfo);
                    }
                }
                continue;
            }
            if (0 == _tcsicmp_insensitive(_T("RomName"), pszLabel) || 
                0 == _tcsicmp_insensitive(_T("ZipName"), pszLabel)) {
                pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                if (NULL == pszInfo) break;
                CopyTrimmedToken(RDI.szZipName, sizeof(RDI.szZipName), TCHARToANSI(pszInfo, NULL, 0));
                log_cb(RETRO_LOG_DEBUG, "[ROMDATA] ZipName: %s\n", RDI.szZipName);
                continue;
            }
            if (0 == _tcsicmp_insensitive(_T("Parent"), pszLabel) || 
                0 == _tcsicmp_insensitive(_T("DrvName"), pszLabel)) {
                pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                if (NULL == pszInfo) break;
                CopyTrimmedToken(RDI.szDrvName, sizeof(RDI.szDrvName), FbneoPublicDrvName(TCHARToANSI(pszInfo, NULL, 0)));
                log_cb(RETRO_LOG_DEBUG, "[ROMDATA] DrvName: %s\n", RDI.szDrvName);
                continue;
            }
            if (0 == _tcsicmp_insensitive(_T("Game"), pszLabel) || 
                0 == _tcsicmp_insensitive(_T("FullName"), pszLabel)) {
                TCHAR szMerger[260] = { 0 };
                INT32 nAdd = 0;
                while (NULL != (pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME))) {
                    _stprintf(szMerger + nAdd, _T("%s "), pszInfo);
                    nAdd = _tcslen(szMerger);
                }
                if (nAdd > 0) szMerger[nAdd - 1] = _T('\0');
                strcpy(szFullName, TCHARToANSI(szMerger, NULL, 0));
                log_cb(RETRO_LOG_DEBUG, "[ROMDATA] FullName: %s\n", szFullName);
                continue;
            }

            // 解析ROM条目
            struct BurnRomInfo ri = { 0 };
            ri.nLen  = UINT32_MAX;
            ri.nCrc  = UINT32_MAX;
            ri.nType = 0U;

            // Nebula格式: name,offset,length,crc,type
            pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
            if (NULL != pszInfo) {
                // 跳过offset字段(我们不使用它)
                pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                if (NULL != pszInfo) {
                    // 解析长度
                    if (_stscanf(pszInfo, _T("%x"), &(ri.nLen)) != 1) {
                        log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 无效的长度格式: %s\n", pszInfo);
                        continue;
                    }
                    
                    if ((UINT32_MAX == ri.nLen) || (0 == ri.nLen)) {
                        log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 无效的ROM长度: 0x%08x\n", ri.nLen);
                        continue;
                    }

                    pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                    if (NULL != pszInfo) {
                        // 解析CRC
                        if (_stscanf(pszInfo, _T("%x"), &(ri.nCrc)) != 1) {
                            log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 无效的CRC格式: %s\n", pszInfo);
                            continue;
                        }
                        
                        if (UINT32_MAX == ri.nCrc && ri.nCrc != 0) {
                            log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 无效的ROM CRC: 0x%08x\n", ri.nCrc);
                            continue;
                        }

                        pszInfo = strqtoken(NULL, DELIM_TOKENS_NAME);
                        if (NULL != pszInfo) {
                            // 解析类型
                            if (_stscanf(pszInfo, _T("%x"), &(ri.nType)) != 1) {
                                ri.nType = 0; // 类型解析失败，使用0
                            }
                        }

                        // 核心改进：根据基板类型和段标记设置ROM类型
                        UINT32 finalType = 0;
                        char romNameA[256] = {0}, sectionA[64] = {0};
                        TCHARToANSI(pszLabel, romNameA, sizeof(romNameA));
                        TCHARToANSI(szRomMask, sectionA, sizeof(sectionA));
                        
                        switch (systemType) {
                            case 1: // Neo基板
                                if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | 1;
                                } 
                                else if (0 == _tcsicmp_insensitive(_T("Text"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA1"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | 2;
                                } 
                                else if (0 == _tcsicmp_insensitive(_T("Graphics"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA2"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | 3;
                                } 
                                else if (0 == _tcsicmp_insensitive(_T("Z80"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SNDA"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | 4;
                                } 
                                else if (0 == _tcsicmp_insensitive(_T("Samples"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SND1"), szRomMask)) 
                                {
                                    finalType = BRF_SND | 5;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("SamplesAES"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SND2"), szRomMask)) 
                                {
                                    // 类型值 6 对应 NeoGeo AES 家用机专用音频样本 典型游戏：nam1975 (Nitro Ball)
                                    finalType = BRF_SND | 6;
                                }
                                // ==== 补充缺失的 BoardPLD 处理 ====
                                else if (0 == _tcsicmp_insensitive(_T("BoardPLD"), szRomMask) || 0 == _tcsicmp_insensitive(_T("OPT1"), szRomMask)) 
                                {
                                    // 板载PLD (可编程逻辑设备) 特殊标记：仅使用 BRF_OPT 标志，无子类型值
                                    finalType = BRF_OPT;
                                }
                                break;

                            case 2: // PGM基板
                            {
                                if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | 1;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Tile"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA1"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | 2; // 背景贴图
                                }
                                else if (0 == _tcsicmp_insensitive(_T("SpriteData"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA2"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | 3; // 精灵数据
                                }
                                else if (0 == _tcsicmp_insensitive(_T("SpriteMasks"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA3"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | 4; // 精灵掩码
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Samples"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SND1"), szRomMask)) 
                                {
                                    finalType = BRF_SND | 5; // 音频样本
                                }
                                else if (0 == _tcsicmp_insensitive(_T("InternalARM7"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG2"), szRomMask)) 
                                {
                                    // ARM7 内部程序 (无 BRF_NODUMP)
                                    finalType = BRF_ESS | BRF_PRG | 7;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Ramdump"), szRomMask)) 
                                {
                                    // 内存转储 (带 BRF_NODUMP 标志)
                                    finalType = BRF_ESS | BRF_PRG | BRF_NODUMP | 7;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("ExternalARM7"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG3"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | 8; // ARM7 外部程序
                                }
                                else if (0 == _tcsicmp_insensitive(_T("ProtectionRom"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG4"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | 9; // 保护芯片ROM
                                }
                                
                                // 特殊处理：Ramdump 长度必须为0
                                if ((finalType & 7) == 7 && (finalType & BRF_NODUMP)) {
                                    if (ri.nLen > 0) {
                                        log_cb(RETRO_LOG_WARN, 
                                            "[ROMDATA] Ramdump长度应为0 (实际:0x%08X)，强制修正", 
                                            ri.nLen);
                                        ri.nLen = 0;
                                    }
                                }
                                break;
                            }
                            
                            case 3: // CPS1基板
                                finalType = GetPlatformAgnosticCps1Type(
                                    RDI.szDrvName,    // 驱动名
                                    romNameA,         // ROM名
                                    ri.nLen,          // 长度
                                    ri.nCrc,          // CRC
                                    sectionA          // 段名
                                );
                                
                                // 动态字节交换调整 (与Windows版兼容)
                                if ((finalType & 0xF) == CPS1_68K_PROGRAM_BYTESWAP) {
                                    if (ri.nLen >= 0x40000) {
                                        finalType = (finalType & ~0xF) | CPS1_68K_PROGRAM_NO_BYTESWAP;
                                        //log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 调整CPS1程序类型为大ROM不交换字节\n");
                                    }
                                } else if ((finalType & 0xF) == CPS1_68K_PROGRAM_NO_BYTESWAP) {
                                    if (ri.nLen < 0x40000) {
                                        finalType = (finalType & ~0xF) | CPS1_68K_PROGRAM_BYTESWAP;
                                        //log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 调整CPS1程序类型为小ROM交换字节\n");
                                    }
                                }
                                break;
                                
                            case 4: // CPS2基板
                                if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | CPS2_PRG_68K;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Graphics"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA1"), szRomMask)) 
                                {
                                    finalType = BRF_GRA | CPS2_GFX;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Z80"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SNDA"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG | CPS2_PRG_Z80;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Samples"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SND1"), szRomMask)) 
                                {
                                    finalType = BRF_SND | CPS2_QSND;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Decryption"), szRomMask) || 0 == _tcsicmp_insensitive(_T("KEY1"), szRomMask)) 
                                {
                                    finalType = CPS2_ENCRYPTION_KEY;
                                }
                                break;
                                
                            case 5: // CPS3基板
                                if (0 == _tcsicmp_insensitive(_T("Bios"), szRomMask)) {
                                    finalType = BRF_ESS | BRF_BIOS;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_PRG;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Graphics"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA1"), szRomMask)) 
                                {
                                    finalType = BRF_GRA;
                                }
                                break;
                                
                            case 6: // Sega Megadrive/Genesis
                            {
                                if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    // 根据ROM文件名特征确定加载方式
                                    TCHAR szLowerName[256] = {0};
                                    _tcscpy(szLowerName, pszLabel);
                                    StringToLower(szLowerName); // 转换为小写便于比较
                                    
                                    // 检测特殊加载方式的关键词
                                    if (_tcsstr(szLowerName, _T("words"))) {
                                        finalType = SEGA_MD_ROM_LOAD16_WORD_SWAP | BRF_PRG | BRF_ESS;
                                    }
                                    else if (_tcsstr(szLowerName, _T("byte"))) {
                                        finalType = SEGA_MD_ROM_LOAD16_BYTE | BRF_PRG | BRF_ESS;
                                    }
                                    else if (_tcsstr(szLowerName, _T("cont40"))) {
                                        finalType = SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000 | BRF_PRG | BRF_ESS;
                                    }
                                    else if (_tcsstr(szLowerName, _T("cont02"))) {
                                        finalType = SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000 | BRF_PRG | BRF_ESS;
                                    }
                                    else {
                                        // 默认加载方式
                                        finalType = SEGA_MD_ROM_LOAD_NORMAL | BRF_PRG | BRF_ESS;
                                    }
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Graphics"), szRomMask) || 0 == _tcsicmp_insensitive(_T("GRA2"), szRomMask)) 
                                {
                                    finalType = BRF_GRA;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Z80"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SNDA"), szRomMask)) 
                                {
                                    finalType = BRF_PRG; // Z80程序
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Samples"), szRomMask) || 0 == _tcsicmp_insensitive(_T("SND1"), szRomMask)) 
                                {
                                    finalType = BRF_SND;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("EEPROM"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_OPT;
                                }
                                break;
                            }
                                
                            default: // 通用处理
                            {
                                // 通用 ROM 类型处理 适用于所有未被特定基板覆盖的情况

                                if (0 == _tcsicmp_insensitive(_T("Program"), szRomMask) || 
                                    0 == _tcsicmp_insensitive(_T("PRG1"), szRomMask)) 
                                {
                                    finalType = BRF_PRG | BRF_ESS;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Graphics"), szRomMask) || 
                                        0 == _tcsicmp_insensitive(_T("GRA1"), szRomMask)) 
                                {
                                    finalType = BRF_GRA;
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Z80"), szRomMask) || 
                                        0 == _tcsicmp_insensitive(_T("SNDA"), szRomMask)) 
                                {
                                    finalType = BRF_PRG; // 声音程序
                                }
                                else if (0 == _tcsicmp_insensitive(_T("Samples"), szRomMask) || 
                                        0 == _tcsicmp_insensitive(_T("SND1"), szRomMask)) 
                                {
                                    finalType = BRF_SND; // 音频样本
                                }
                                else if (0 == _tcsicmp_insensitive(_T("BoardPLD"), szRomMask) || 
                                        0 == _tcsicmp_insensitive(_T("OPT1"), szRomMask)) 
                                {
                                    finalType = BRF_OPT; // 板载逻辑
                                }
                                else if (0 == _tcsicmp_insensitive(_T("EEPROM"), szRomMask)) 
                                {
                                    finalType = BRF_ESS | BRF_OPT; // 保存数据
                                }
                                else 
                                {
                                    // 最终后备：使用原始类型值
                                    finalType = ri.nType;
                                    log_cb(RETRO_LOG_WARN, 
                                        "[ROMDATA] ⚠️ 未知ROM类型: 基板=%d, 段=%s, 使用原始类型: 0x%08X",
                                        systemType, szRomMask, ri.nType);
                                }
                                break;
                            }
                        }
                        
                        // 如果通过基板逻辑确定了类型，则覆盖原始类型
                        if (finalType > 0) {
                            ri.nType = finalType;
                            //log_cb(RETRO_LOG_DEBUG, "[FBNeo] 🔍 基板逻辑确定类型: 0x%08x\n", ri.nType);
                        } else if (ri.nType > 0) {
                            //log_cb(RETRO_LOG_DEBUG, "[FBNeo] 🔍 使用DAT文件中的原始类型: 0x%08x\n", ri.nType);
                        }

                        if (ri.nType > 0U) {
                            RDI.nDescCount++;

                            if (NULL != pDataRomDesc) {
                                pDataRomDesc[RDI.nDescCount].szName = (char*)malloc(512);
                                memset(pDataRomDesc[RDI.nDescCount].szName, 0, sizeof(char) * 512);
                                strcpy(pDataRomDesc[RDI.nDescCount].szName, TCHARToANSI(pszLabel, NULL, 0));

                                pDataRomDesc[RDI.nDescCount].nLen  = ri.nLen;
                                pDataRomDesc[RDI.nDescCount].nCrc  = ri.nCrc;
                                pDataRomDesc[RDI.nDescCount].nType = ri.nType;

                                log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 解析ROM条目: %s, 长度: 0x%08x, CRC: 0x%08x\n", 
                                    pDataRomDesc[RDI.nDescCount].szName, 
                                    pDataRomDesc[RDI.nDescCount].nLen,
                                    pDataRomDesc[RDI.nDescCount].nCrc);
                                    //pDataRomDesc[RDI.nDescCount].nType);, 类型: 0x%08x
                            }
                        } else {
                            log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 无法确定ROM类型，跳过条目: %s\n", pszLabel);
                        }
                    }
                }
            }
        }
    }

    log_cb(RETRO_LOG_INFO, "[ROMDATA] ✅ 解析Nebula格式ROMDATA完成，共 %d 个ROM条目\n", RDI.nDescCount + 1);
    return RDI.nDescCount;
}

// 解析标准格式ROMDATA
static INT32 ParseStandardRomData(FILE* fp)
{
	TCHAR szBuf[MAX_PATH] = { 0 };
	TCHAR* pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = SkipUtfBom(szBuf);
			pszLabel = strqtoken(pszBuf, " \t\r\n,%:|{}");

			if (NULL == pszLabel) continue;
			if ((_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;
			if (0 == _tcsicmp(_T("ZipName"), pszLabel)) {
				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if (NULL == pszInfo) break; // 未指定ROM集
				if (NULL != pDataRomDesc) {
					CopyTrimmedToken(RDI.szZipName, sizeof(RDI.szZipName), TCHARToANSI(pszInfo, NULL, 0));
					log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 设置ZipName: %s\n", RDI.szZipName);
				}
			}
			if (0 == _tcsicmp(_T("DrvName"), pszLabel)) {
				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if (NULL == pszInfo) break; // 未指定驱动
				if (NULL != pDataRomDesc) {
					CopyTrimmedToken(RDI.szDrvName, sizeof(RDI.szDrvName), FbneoPublicDrvName(TCHARToANSI(pszInfo, NULL, 0)));
					log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 设置DrvName: %s\n", RDI.szDrvName);
				}
			}
			if (0 == _tcsicmp(_T("ExtraRom"), pszLabel)) {
				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if ((NULL != pszInfo) && (NULL != pDataRomDesc)) {
					strcpy(RDI.szExtraRom, TCHARToANSI(pszInfo, NULL, 0));
					log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 设置ExtraRom: %s\n", RDI.szExtraRom);
				}
			}
			if (0 == _tcsicmp(_T("FullName"), pszLabel)) {
				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if ((NULL != pszInfo) && (NULL != pDataRomDesc)) {
					strcpy(szFullName, TCHARToANSI(pszInfo, NULL, 0));
					log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 设置FullName: %s\n", szFullName);
				}
			}

			// romname, len, crc, type
			struct BurnRomInfo ri = { 0 };
			ri.nLen  = UINT32_MAX;
			ri.nCrc  = UINT32_MAX;
			ri.nType = 0;

			pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
			if (NULL != pszInfo) {
				_stscanf(pszInfo, _T("%x"), &(ri.nLen));
				if (UINT32_MAX == ri.nLen) continue;

				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if (NULL != pszInfo) {
					_stscanf(pszInfo, _T("%x"), &(ri.nCrc));
					if (UINT32_MAX == ri.nCrc) continue;

					while (NULL != (pszInfo = strqtoken(NULL, " \t\r\n,%:|{}"))) {
						INT32 nValue = -1;

						if (0 == _tcscmp(pszInfo, _T("BRF_PRG"))) {
							ri.nType |= BRF_PRG;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_GRA"))) {
							ri.nType |= BRF_GRA;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_SND"))) {
							ri.nType |= BRF_SND;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_ESS"))) {
							ri.nType |= BRF_ESS;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_BIOS"))) {
							ri.nType |= BRF_BIOS;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_SELECT"))) {
							ri.nType |= BRF_SELECT;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_OPT"))) {
							ri.nType |= BRF_OPT;
							continue;
						}
						if (0 == _tcscmp(pszInfo, _T("BRF_NODUMP"))) {
							ri.nType |= BRF_NODUMP;
							continue;
						}
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_68K_PROGRAM_BYTESWAP")))) {			//  1
                            ri.nType |= 1;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K_SIMM"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_68K_PROGRAM_NO_BYTESWAP")))) {		//  2
                            ri.nType |= 2;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K_XOR_TABLE"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_Z80_PROGRAM")))) {					//  3
                            ri.nType |= 3;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("CPS1_TILES"))) {							//  4
                            ri.nType |= 4;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_OKIM6295_SAMPLES")))) {				//  5
                            ri.nType |= 5;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SIMM"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_QSOUND_SAMPLES")))) {				//  6
                            ri.nType |= 6;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SPLIT4"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_PIC")))) {							//  7
                            ri.nType |= 7;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SPLIT8"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2EBBL_400000")))) {	//  8
                            ri.nType |= 8;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_19XXJ"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_400000")))) {			//  9
                            ri.nType |= 9;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_Z80"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2KORYU_400000")))) {	// 10
                            ri.nType |= 10;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2B_400000"))) {		// 11
                            ri.nType |= 11;
                            continue;
                        }
                        if ((0 == _tcscmp(pszInfo, _T("CPS2_QSND"))) ||
                            (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2MKOT_400000")))) {	// 12
                            ri.nType |= 12;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("CPS2_QSND_SIMM"))) {						// 13
                            ri.nType |= 13;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("CPS2_QSND_SIMM_BYTESWAP"))) {				// 14
                            ri.nType |= 14;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("CPS2_ENCRYPTION_KEY"))) {					// 15
                            ri.nType |= 15;
                            continue;
                        }
                        // megadrive
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD_NORMAL"))) {
                            ri.nType |= 0x10;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_WORD_SWAP"))) {
                            ri.nType |= 0x20;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_BYTE"))) {
                            ri.nType |= 0x30;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000"))) {
                            ri.nType |= 0x40;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000"))) {
                            ri.nType |= 0x50;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_000000"))) {
                            ri.nType |= 0x01;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_000001"))) {
                            ri.nType |= 0x02;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_020000"))) {
                            ri.nType |= 0x03;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_080000"))) {
                            ri.nType |= 0x04;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_100000"))) {
                            ri.nType |= 0x05;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_100001"))) {
                            ri.nType |= 0x06;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_200000"))) {
                            ri.nType |= 0x07;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_300000"))) {
                            ri.nType |= 0x08;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_RELOAD_200000_200000"))) {
                            ri.nType |= 0x09;
                            continue;
                        }
                        if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_RELOAD_100000_300000"))) {
                            ri.nType |= 0x0a;
                            continue;
                        }
						_stscanf(pszInfo, _T("%x"), &nValue);
						if (-1 != nValue) {
							ri.nType |= (UINT32)nValue;
							continue;
						}
					}

					if (ri.nType > 0) {
						RDI.nDescCount++;

						if (NULL != pDataRomDesc) {
							pDataRomDesc[RDI.nDescCount].szName = (char*)malloc(512);

							strcpy(pDataRomDesc[RDI.nDescCount].szName, TCHARToANSI(pszLabel, NULL, 0));

							pDataRomDesc[RDI.nDescCount].nLen  = ri.nLen;
							pDataRomDesc[RDI.nDescCount].nCrc  = ri.nCrc;
							pDataRomDesc[RDI.nDescCount].nType = ri.nType;

							log_cb(RETRO_LOG_DEBUG, "[ROMDATA] 添加ROM条目: %s, 长度: 0x%08x, CRC: 0x%08x\n", 
								pDataRomDesc[RDI.nDescCount].szName, 
								pDataRomDesc[RDI.nDescCount].nLen,
								pDataRomDesc[RDI.nDescCount].nCrc,
								pDataRomDesc[RDI.nDescCount].nType);
						}
					}
				}
			}
		}
	}

	return RDI.nDescCount;
}

static INT32 LoadRomdata()
{
    RDI.nDescCount = -1; // 初始化为失败状态

    INT32 nType = IsDatUTF8BOM();
    if (-1 == nType) {
        log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 无法打开或读取ROMDATA文件: %s\n", szRomdataName);
        return RDI.nDescCount;
    }

    const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");
    FILE* fp = _tfopen(szRomdataName, szReadMode);
    if (NULL == fp) {
        log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 无法打开ROMDATA文件: %s\n", szRomdataName);
        return RDI.nDescCount;
    }

    log_cb(RETRO_LOG_INFO, "[ROMDATA] ✅ 开始加载ROMDATA文件: %s\n", szRomdataName);

    // 更精确的Nebula格式检测
    bool bIsNebulaFormat = false;
    char szFirstLines[1024] = {0};
    
    // 读取文件前512字节用于格式检测
    size_t bytesRead = fread(szFirstLines, 1, sizeof(szFirstLines)-1, fp);
    fseek(fp, 0, SEEK_SET);
    
    // Nebula格式特征检测
    if (bytesRead > 0) {
        // 检查是否包含Nebula特有的标记
        const char* nebulaMarkers[] = {
            "System:", 
            "RomName:", 
            "[Program]", 
            "[Text]", 
            "[Z80]", 
            "[Samples]", 
            "[Graphics]",
            "[System]"
        };
        
        for (size_t i = 0; i < sizeof(nebulaMarkers)/sizeof(nebulaMarkers[0]); i++) {
            if (strstr(szFirstLines, nebulaMarkers[i]) != NULL) {
                bIsNebulaFormat = true;
                break;
            }
        }
    }

    if (bIsNebulaFormat) {
        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到Nebula格式ROMDATA\n");
        ParseNebulaRomData(fp);
    } else {
        log_cb(RETRO_LOG_INFO, "[ROMDATA] 检测到标准格式ROMDATA\n");
        ParseStandardRomData(fp);
    }

    fclose(fp);

    if (RDI.nDescCount >= 0) {
        log_cb(RETRO_LOG_INFO, "[ROMDATA] ✅ 成功加载ROMDATA，共 %d 个ROM条目\n", RDI.nDescCount + 1);
    } else {
        log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 加载ROMDATA失败\n");
    }

    return RDI.nDescCount;
}

extern "C" const char* RomDataGetZipName()
{
	return RDI.szZipName;
}

extern "C" int RomDataGetDrvIndex(const char* zipname)
{
	if (zipname == NULL || zipname[0] == '\0') return -1;
	return BurnDrvGetIndex((char*)zipname);
}

char* RomdataGetDrvName()
{
	INT32 nType = IsDatUTF8BOM();
	if (-1 == nType) return NULL;

	const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");

	FILE* fp = _tfopen(szRomdataName, szReadMode);
	if (NULL == fp) return NULL;

	memset(szRomset, 0, MAX_PATH * sizeof(TCHAR));

	TCHAR szBuf[MAX_PATH] = { 0 }, * pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = SkipUtfBom(szBuf);

			pszLabel = strqtoken(pszBuf, " \t\r\n,%:|{}");
			if (NULL == pszLabel) continue;
			if ((_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;

			if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel)) {
				pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
				if (NULL == pszInfo) break; // 未指定驱动

				fclose(fp);
				_tcscpy(szRomset, TCHARToANSI(pszInfo, NULL, 0));

				return szRomset;
                break;
			}
		}
	}
	fclose(fp);
	return NULL;
}

INT32 create_variables_from_romdatas()
{
	INT32 nRet = 0;

	bool bRomdataMode = (NULL != pDataRomDesc);
	const char* pszDrvName = bRomdataMode ? RDI.szDrvName : BurnDrvGetTextA(DRV_NAME), * pszExt = ".dat";
	if (NULL == pszDrvName) return -2;

	romdata_core_options.clear();
	CoreRomDataPathsLoad();

	for (INT32 i = 0; i < DIRS_MAX; i++)
	{
		if (0 == _tcscmp("", CoreRomDataPaths[i])) continue;

		char* p = find_last_slash(CoreRomDataPaths[i]);
		if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';

		TCHAR szFilePathSearch[MAX_PATH] = { 0 }, szDatPaths[MAX_PATH] = { 0 };
		snprintf(szFilePathSearch, MAX_PATH - 1, "%s%c", CoreRomDataPaths[i], PATH_DEFAULT_SLASH_C());

		// romdata_dirs/
		strcpy(szDatPaths, szFilePathSearch);

		struct RDIR* entry = retro_opendir_include_hidden(szFilePathSearch, true);
		if (!entry || retro_dirent_error(entry)) continue;

		FILE* fp = NULL;

		while (retro_readdir(entry))
		{
			const char* name = retro_dirent_get_name(entry);

			if (
				retro_dirent_is_dir(entry, NULL) ||
				(0 != strcmp(pszExt, strrchr(name, '.'))) ||
				(0 == strcmp(name, ".")) || (0 == strcmp(name, ".."))
				)
				continue;

			memset(szFilePathSearch, 0, MAX_PATH * sizeof(TCHAR));
			snprintf(szFilePathSearch, MAX_PATH - 1, "%s%s", szDatPaths, name);

			fp = fopen(szFilePathSearch, "rb");
			if (NULL == fp) continue;

			// get dat size
			fseek(fp, 0, SEEK_END);
			INT32 nDatSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* pszTest = (char*)malloc(nDatSize + 1);
			if (NULL == pszTest)
			{
				fclose(fp);
				continue;
			}

			memset(pszTest, 0, nDatSize + 1);
			fread(pszTest, nDatSize, 1, fp);
			INT32 nType = IsUTF8Text(pszTest, nDatSize);
			free(pszTest);
			pszTest = NULL;
			fclose(fp);

			const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");
			if (NULL == (fp = _tfopen(szFilePathSearch, szReadMode))) continue;

			TCHAR szBuf[MAX_PATH] = { 0 }, * pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;
			bool bDrvOK = false, bDescOK = false;

			TCHAR szLocalAltName[MAX_PATH] = { 0 };  // 重命名以避免阴影警告
			TCHAR szLocalAltDesc[MAX_PATH] = { 0 };  // 重命名以避免阴影警告

			while (!feof(fp))
			{
				if (NULL != _fgetts(szBuf, MAX_PATH, fp))
				{
					pszBuf = SkipUtfBom(szBuf);
					pszLabel = strqtoken(pszBuf, " \t\r\n,%:|{}");
					if ((NULL == pszLabel) || (_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;

					if (0 == _tcsicmp(_T("DrvName"), pszLabel) || 0 == _tcsicmp(_T("Parent"), pszLabel))
					{
						pszInfo = strqtoken(NULL, " \t\r\n,%:|{}");
						if ((NULL == pszInfo) || !FbneoDrvNamesMatch(pszDrvName, pszInfo)) { bDrvOK = false; break; } // 不匹配当前驱动
						_tcscpy(szLocalAltName, FbneoPublicDrvName(pszInfo)); bDrvOK = true;
					}
					if (0 == _tcsicmp(_T("FullName"), pszLabel))
					{
						const TCHAR* pDesc = (NULL == (pszInfo = strqtoken(NULL, " \t\r\n,%:|{}"))) ? szFilePathSearch : pszInfo;
						_tcscpy(szLocalAltDesc, pDesc); bDescOK = true;
					}
					if (bDrvOK && bDescOK)
					{
						char szKey[100] = { 0 };
						snprintf(szKey, 100 - 1, "fbneo-romdata-%s-%d", pszDrvName, nRet);

						romdata_core_options.push_back(romdata_core_option());
						romdata_core_option* romdata_option = &romdata_core_options.back();
						romdata_option->dat_path            = szFilePathSearch;
						romdata_option->option_name         = szKey;
						romdata_option->friendly_name       = szLocalAltDesc;

						nRet++; break;
					}
				}
			}
			fclose(fp);
		}
		retro_closedir(entry);
	}

	return nRet;
}

INT32 reset_romdatas_from_variables()
{
    struct retro_variable var = { 0 };

    for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
    {
        romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];

        var.key = romdata_option->option_name.c_str();
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
            continue;

        var.value = "disabled";

        if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
            return -1;
    }

    return 0;
}

INT32 apply_romdatas_from_variables()
{
    if (!bPatchedRomsetsEnabled) {
        reset_romdatas_from_variables();
        return -2;
    }

    INT32 nIndex = 0, nCount = 0;
    struct retro_variable var = { 0 };

    // Calculate the number of selections.
    for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
    {
        romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
        var.key = romdata_option->option_name.c_str();

        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
            continue;

        if (0 == strcmp(var.value, "enabled")) nCount++;
    }

    // If nCount is 0, no romdata is enabled and we should return -1
    if (nCount == 0)
        return -1;

    // Generates a random value when multiple selections are made.
    if (nCount > 1)
        nIndex = rand() % nCount;

    nCount = 0;

    // Selects a romdata by default/random value.
    for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
    {
        romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
        var.key = romdata_option->option_name.c_str();

        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
            continue;

        if (0 == strcmp(var.value, "enabled"))
        {
            if (nIndex != nCount++) continue;

            RomDataExit();  // Reset the parasitized drvname.
            _tcscpy(szRomdataName, romdata_option->dat_path.c_str());
        }
    }

    // Reset
    for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
    {
        romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
        var.key = romdata_option->option_name.c_str();

        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
            continue;

        if (0 == strcmp(var.value, "enabled"))
        {
            var.value = "disabled";

            if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
                return -1;
        }
    }

    return nIndex;
}

void RomDataInit()
{
    // 保存当前状态，防止加载失败时数据损坏
    RomDataInfo originalRDI = RDI;
    char originalZipName[MAX_PATH] = {0}, originalDrvName[MAX_PATH] = {0};
    if (RDI.szZipName[0]) strncpy(originalZipName, RDI.szZipName, sizeof(originalZipName)-1);
    if (RDI.szDrvName[0]) strncpy(originalDrvName, RDI.szDrvName, sizeof(originalDrvName)-1);

    // 确保没有残留的ROM描述数据
    if (pDataRomDesc) {
        RomDataExit();
    }

    // 第一次调用：获取ROM条目数量
    INT32 nLen = LoadRomdata();
    
    if (nLen >= 0 && pDataRomDesc == NULL) {
        // 分配足够的内存空间 (+1 用于结束标记)
        pDataRomDesc = (struct BurnRomInfo*)malloc((nLen + 1) * sizeof(BurnRomInfo));
        if (pDataRomDesc) {
            // 初始化内存
            memset(pDataRomDesc, 0, (nLen + 1) * sizeof(BurnRomInfo));
            
            // 第二次调用：填充数据
            if (LoadRomdata() < 0) {  // 检查加载是否失败
                log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 第二次加载ROMDATA失败\n");
                
                // 清理已分配的内存
                for (int i = 0; i < nLen + 1; i++) {
                    if (pDataRomDesc[i].szName) {
                        free(pDataRomDesc[i].szName);
                    }
                }
                free(pDataRomDesc);
                pDataRomDesc = NULL;
                
                // 恢复原始状态
                RDI = originalRDI;
                strncpy(RDI.szZipName, originalZipName, sizeof(RDI.szZipName)-1);
                strncpy(RDI.szDrvName, originalDrvName, sizeof(RDI.szDrvName)-1);
                
                return;
            }
            
            // 匹配游戏驱动
            RDI.nDriverId = BurnDrvGetIndex(RDI.szDrvName);
            if (RDI.nDriverId == -1) {
                RDI.nDriverId = BurnDrvGetIndex(RDI.szZipName);
                if (RDI.nDriverId != -1) {
                    strncpy(RDI.szDrvName, RDI.szZipName, sizeof(RDI.szDrvName)-1);
                    log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 使用ZipName作为DrvName: %s\n", RDI.szDrvName);
                }
            }
            
            // 设置当前驱动
            if (RDI.nDriverId != -1) {
                BurnDrvSetZipName(RDI.szZipName, RDI.nDriverId);
                nBurnDrvActive = RDI.nDriverId;
                log_cb(RETRO_LOG_INFO, "[ROMDATA] ✅ 设置活动驱动: %s (ID: %d)\n", 
                       RDI.szDrvName, RDI.nDriverId);
            } else {
                log_cb(RETRO_LOG_WARN, "[ROMDATA] ⚠️ 暂未解析到驱动ID，稍后将继续尝试: %s/%s\n", 
                       RDI.szDrvName, RDI.szZipName);
            }
        } else {
            log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 内存分配失败: %d 个ROM条目\n", nLen + 1);
        }
    } else if (nLen < 0) {
        log_cb(RETRO_LOG_ERROR, "[ROMDATA] ❌ 初始加载ROMDATA失败\n");
    }
    
    // 如果加载失败但原始状态有效，尝试恢复默认驱动
    if (pDataRomDesc == NULL && originalRDI.nDriverId != -1) {
        nBurnDrvActive = originalRDI.nDriverId;
        log_cb(RETRO_LOG_INFO, "[ROMDATA] 恢复原始驱动: ID %d\n", originalRDI.nDriverId);
    }
}

void RomDataSetFullName()
{
#if 0
	// At this point, the driver's default ZipName has been replaced with the ZipName from the rom data
	RDI.nDriverId = BurnDrvGetIndex(RDI.szZipName);

	if (-1 != RDI.nDriverId) {
		wchar_t* szOldName = BurnDrvGetFullNameW(RDI.nDriverId);
		memset(RDI.szOldName, '\0', sizeof(RDI.szOldName));

		if (0 != wcscmp(szOldName, RDI.szOldName)) {
			wcscpy(RDI.szOldName, szOldName);
		}

		BurnDrvSetFullNameW(RDI.szFullName, RDI.nDriverId);
	}
#endif
}

void RomDataExit()
{
	if (NULL != pDataRomDesc) {

		for (int i = 0; i < RDI.nDescCount + 1; i++) {
			if (pDataRomDesc[i].szName) {
				free(pDataRomDesc[i].szName);
				pDataRomDesc[i].szName = NULL;
			}
		}

		free(pDataRomDesc);
		pDataRomDesc = NULL;

		// 恢复原始 ZipName（如果 driver id 有效）
		if (-1 != RDI.nDriverId) {
			if (RDI.szDrvName[0]) {
				BurnDrvSetZipName(RDI.szDrvName, RDI.nDriverId);
			}
			RDI.nDriverId = -1;
		}

		memset(&RDI, 0, sizeof(RomDataInfo));
		memset(szRomdataName, 0, sizeof(szRomdataName));

		RDI.nDescCount = -1;
	}
}
