/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba-util/formatting.h>

#include <float.h>

int ftostr_l(char* restrict str, size_t size, float f, mLocale_t locale) {
#if defined(HAVE_SNPRINTF_L) && HAVE_SNPRINTF_L
	return snprintf_l(str, size, locale, "%.*g", FLT_DIG, f);
#elif HAVE_USELOCALE
	locale_t old = uselocale((locale_t) locale);
	int res = snprintf(str, size, "%.*g", FLT_DIG, f);
	uselocale(old);
	return res;
#elif defined(HAVE_SETLOCALE) && HAVE_SETLOCALE
	char* old = setlocale(LC_NUMERIC, (const char*) locale);
	int res = snprintf(str, size, "%.*g", FLT_DIG, f);
	setlocale(LC_NUMERIC, old);
	return res;
#else
	UNUSED(locale);
	return snprintf(str, size, "%.*g", FLT_DIG, f);
#endif
}

#if !defined(HAVE_STRTOF_L) || !HAVE_STRTOF_L
float strtof_l(const char* restrict str, char** restrict end, mLocale_t locale) {
#if HAVE_USELOCALE
	locale_t old = uselocale((locale_t) locale);
	float res = strtof(str, end);
	uselocale(old);
	return res;
#elif defined(HAVE_SETLOCALE) && HAVE_SETLOCALE
	char* old = setlocale(LC_NUMERIC, (const char*) locale);
	float res = strtof(str, end);
	setlocale(LC_NUMERIC, old);
	return res;
#else
	UNUSED(locale);
	return strtof(str, end);
#endif
}
#endif

int ftostr_u(char* restrict str, size_t size, float f) {
#if HAVE_LOCALE
	mLocale_t l = newlocale(LC_NUMERIC_MASK, "C", 0);
#elif HAVE_LOCALE_T
	mLocale_t l = (mLocale_t) 0;
#else
	mLocale_t l = "C";
#endif
	int res = ftostr_l(str, size, f, l);
#if HAVE_LOCALE
	freelocale((locale_t) l);
#endif
	return res;
}

float strtof_u(const char* restrict str, char** restrict end) {
#if HAVE_LOCALE
	mLocale_t l = newlocale(LC_NUMERIC_MASK, "C", 0);
#elif HAVE_LOCALE_T
	mLocale_t l = (mLocale_t) 0;
#else
	mLocale_t l = "C";
#endif
	float res = strtof_l(str, end, l);
#if HAVE_LOCALE
	freelocale((locale_t) l);
#endif
	return res;
}

#if !defined(HAVE_LOCALTIME_R) || !HAVE_LOCALTIME_R
struct tm* localtime_r(const time_t* t, struct tm* date) {
#ifdef _WIN32
	localtime_s(date, t);
	return date;
#elif defined(PSP2)
	extern struct tm* sceKernelLibcLocaltime_r(const time_t* t, struct tm* date);
	return sceKernelLibcLocaltime_r(t, date);
#else
#warning localtime_r not emulated on this platform
	return 0;
#endif
}
#endif
