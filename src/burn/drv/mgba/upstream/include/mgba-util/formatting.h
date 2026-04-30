/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef FORMATTING_H
#define FORMATTING_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include "locale.h"

#ifndef HAVE_LOCALE
#if defined(__ANDROID__)
#if defined(__ANDROID_API__) && (__ANDROID_API__ >= 21)
#define HAVE_LOCALE 1
#else
#define HAVE_LOCALE 0
#endif
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__HAIKU__) || defined(__CYGWIN__)
#define HAVE_LOCALE 1
#else
#define HAVE_LOCALE 0
#endif
#endif

#ifndef HAVE_USELOCALE
#if HAVE_LOCALE && !defined(_WIN32)
#define HAVE_USELOCALE 1
#else
#define HAVE_USELOCALE 0
#endif
#endif

#ifndef HAVE_LOCALE_T
#if HAVE_LOCALE || defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__HAIKU__) || defined(__CYGWIN__)
#define HAVE_LOCALE_T 1
#else
#define HAVE_LOCALE_T 0
#endif
#endif

#ifndef HAVE_STRTOF_L
#if HAVE_LOCALE_T && !defined(_WIN32)
#define HAVE_STRTOF_L 1
#else
#define HAVE_STRTOF_L 0
#endif
#endif

#if defined(HAVE_XLOCALE) && HAVE_XLOCALE
#include <xlocale.h>
#endif

#if HAVE_LOCALE_T
typedef locale_t mLocale_t;
#else
typedef const char* mLocale_t;
#endif

int ftostr_l(char* restrict str, size_t size, float f, mLocale_t locale);

#if !defined(HAVE_STRTOF_L) || !HAVE_STRTOF_L
float strtof_l(const char* restrict str, char** restrict end, mLocale_t locale);
#endif

int ftostr_u(char* restrict str, size_t size, float f);
float strtof_u(const char* restrict str, char** restrict end);

#if !defined(HAVE_LOCALTIME_R) || !HAVE_LOCALTIME_R
struct tm* localtime_r(const time_t* timep, struct tm* result);
#endif

CXX_GUARD_END

#endif
