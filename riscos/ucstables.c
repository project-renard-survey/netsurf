/*
 * Copyright 2005 John M Bell <jmb202@ecs.soton.ac.uk>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 * UCS conversion tables and RISC OS-specific UTF-8 text handling
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <oslib/osbyte.h>
#include <oslib/territory.h>

#include "utils/config.h"
#include "riscos/ucstables.h"
#include "utils/log.h"
#include "utils/utf8.h"
#include "utils/utils.h"

/* Common values (ASCII) */
#define common								\
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,	\
   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,	\
   32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,	\
   48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,	\
   64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,	\
   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,	\
   96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,	\
  112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127	\

/* 0x8c->0x9F, used by many of the encodings */
#define common2								\
  0x2026, 0x2122, 0x2030, 0x2022, 0x2018, 0x2019, 0x2039, 0x203a,	\
  0x201c, 0x201d, 0x201e, 0x2013, 0x2014, 0x2212, 0x0152, 0x0153,	\
  0x2020, 0x2021, 0xfb01, 0xfb02

static const int latin1_table[256] =
{
  common,
  0x20ac, 0x0174, 0x0175, -1, -1, 0x0176, 0x0177, -1, -1, -1, -1, -1,
  common2,
  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

static const int latin2_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7,
  0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
  0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7,
  0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
};

static const int latin3_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, -1, 0x0124, 0x00A7,
  0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, -1, 0x017B,
  0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7,
  0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, -1, 0x017C,
  0x00C0, 0x00C1, 0x00C2, -1, 0x00C4, 0x010A, 0x0108, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  -1, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7,
  0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, -1, 0x00E4, 0x010B, 0x0109, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  -1, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7,
  0x011D, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9
};

static const int latin4_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7,
  0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
  0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7,
  0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
  0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
  0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9
};

static const int latin5_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF
};

static const int latin6_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7,
  0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
  0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7,
  0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2015, 0x016B, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138
};

static const int latin7_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0x2026, 0x2122, 0x2030, 0x2022, 0x2018, -1, 0x2039, 0x203a,
  -1, -1, -1, 0x2013, 0x2014, 0x2212, 0x0152, 0x0153,
  0x2020, 0x2021, 0xfb01, 0xfb02,
  0x00A0, 0x201D, 0x00A2, 0x00A3, 0x00A4, 0x201E, 0x00A6, 0x00A7,
  0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x201C, 0x00B5, 0x00B6, 0x00B7,
  0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
  0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
  0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
  0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
  0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
  0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
  0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
  0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
  0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x2019
};

static const int latin8_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x1E02, 0x1E03, 0x00A3, 0x010A, 0x010B, 0x1E0A, 0x00A7,
  0x1E80, 0x00A9, 0x1E82, 0x1E0B, 0x1EF2, 0x00AD, 0x00AE, 0x0178,
  0x1E1E, 0x1E1F, 0x0120, 0x0121, 0x1E40, 0x1E41, 0x00B6, 0x1E56,
  0x1E81, 0x1E57, 0x1E83, 0x1E60, 0x1EF3, 0x1E84, 0x1E85, 0x1E61,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0174, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x1E6A,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x0176, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x0175, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x1E6B,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x0177, 0x00FF
};

static const int latin9_table[256] =
{
  common,
  -1, 0x0174, 0x0175, -1, -1, 0x0176, 0x0177, -1, -1, -1, -1, -1,
  0x2026, 0x2122, 0x2030, 0x2022, 0x2018, 0x2019, 0x2039, 0x203a,
  0x201c, 0x201d, 0x201e, 0x2013, 0x2014, 0x2212, -1, -1,
  0x2020, 0x2021, 0xfb01, 0xfb02,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7,
  0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7,
  0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
};

static const int latin10_table[256] =
{
  common,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0x2026, 0x2122, 0x2030, 0x2022, 0x2018, 0x2019, 0x2039, 0x203a,
  0x201c, -1, -1, 0x2013, 0x2014, 0x2212, -1, -1,
  0x2020, 0x2021, 0xfb01, 0xfb02,
  0x00A0, 0x0104, 0x0105, 0x0141, 0x20AC, 0x201E, 0x0160, 0x00a7,
  0x0161, 0x00A9, 0x0218, 0x00AB, 0x0179, 0x00AD, 0x017A, 0x017B,
  0x00B0, 0x00B1, 0x010C, 0x0142, 0x017D, 0x201D, 0x00B6, 0x00B7,
  0x017E, 0x010D, 0x0219, 0x00BB, 0x0152, 0x0153, 0x0178, 0x017C,
  0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0106, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0110, 0x0143, 0x00D2, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x015A,
  0x0170, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0118, 0x021A, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x0107, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x0111, 0x0144, 0x00F2, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x015B,
  0x0171, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0119, 0x021B, 0x00FF
};

static const int welsh_table[256] =
{
  common,
  0x20ac, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  common2,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x1E80, 0x00A9, 0x1E82, 0x00AB, 0x1EF2, 0x00AD, 0x00AE, 0x0178,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x1E81, 0x00B9, 0x1E83, 0x00BB, 0x1EF3, 0x1E84, 0x1E85, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0174, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x0176, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x0175, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x0177, 0x00FF
};

static const int greek_table[256] =
{
  common,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0x00A0, 0x2018, 0x2019, 0x00A3, 0x20AC, 0x20AF, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x037A, 0x00AB, 0x00AC, 0x00AD, 0x037E, 0x2015,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x0385, 0x0386, 0x0387,
  0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
  0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
  0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, -1, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
  0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
  0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
  0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
  0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
  0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, -1
};

static const int cyrillic_table[256] =
{
  common,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
  0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
  0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
  0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
  0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
  0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
  0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
  0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F
};

static const int hebrew_table[256] =
{
  common,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0x00A0, -1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x203E,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 0x2017,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
  0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
  0x05E8, 0x05E9, 0x05EA, -1, -1, 0x200E, 0x200F, -1
};

/**
 * Retrieve UCS table (above), given alphabet number
 *
 * \param alphabet  The RISC OS alphabet number
 * \return pointer to table, or NULL if not found
 */
const int *ucstable_from_alphabet(int alphabet)
{
	const int *ucstable = NULL;

	switch (alphabet) {
		case territory_ALPHABET_LATIN1:
			ucstable = latin1_table;
			break;
		case territory_ALPHABET_LATIN2:
			ucstable = latin2_table;
			break;
		case territory_ALPHABET_LATIN3:
			ucstable = latin3_table;
			break;
		case territory_ALPHABET_LATIN4:
			ucstable = latin4_table;
			break;
		case territory_ALPHABET_LATIN5:
			ucstable = latin5_table;
			break;
		case territory_ALPHABET_LATIN6:
			ucstable = latin6_table;
			break;
		case 114: /* Latin7 */
			ucstable = latin7_table;
			break;
		case 115: /* Latin8 */
			ucstable = latin8_table;
			break;
		case 116: /* Latin10 */
			ucstable = latin10_table;
			break;
		case territory_ALPHABET_LATIN9:
			ucstable = latin9_table;
			break;
		case territory_ALPHABET_WELSH:
			ucstable = welsh_table;
			break;
		case territory_ALPHABET_GREEK:
			ucstable = greek_table;
			break;
		case territory_ALPHABET_CYRILLIC:
			ucstable = cyrillic_table;
			break;
		case territory_ALPHABET_HEBREW:
			ucstable = hebrew_table;
			break;
		default:
			ucstable = NULL;
			break;
	}

	return ucstable;
}


static const char *localencodings[] = {
	"ISO-8859-1//TRANSLIT",	/* BFont - 100 - just use Latin1, instead */
	"ISO-8859-1//TRANSLIT",
	"ISO-8859-2//TRANSLIT",
	"ISO-8859-3//TRANSLIT",
	"ISO-8859-4//TRANSLIT",
	"ISO-8859-5//TRANSLIT",
	"ISO-8859-6//TRANSLIT",
	"ISO-8859-7//TRANSLIT",
	"ISO-8859-8//TRANSLIT",
	"ISO-8859-9//TRANSLIT",
	"ISO-IR-182//TRANSLIT",
	"UTF-8",
	"ISO-8859-15//TRANSLIT",
	"ISO-8859-10//TRANSLIT",
	"ISO-8859-13//TRANSLIT",
	"ISO-8859-14//TRANSLIT",
	"ISO-8859-16//TRANSLIT",
#define CONT_ENC_END 116	/* RISC OS alphabet numbers lie in a
				 * contiguous range [100,CONT_ENC_END]
				 * _except_ for Cyrillic2, which doesn't.
				 */
	"CP866//TRANSLIT"	/* Cyrillic2 - 120 */
};

static const struct special {
	char local;		/**< Local 8bit representation */
	char len;		/**< Length (in bytes) of UTF-8 character */
	const char *utf;	/**< UTF-8 representation */
} special_chars[] = {
	{ 0x80, 3, "\xE2\x82\xAC" },	/* EURO SIGN */
	{ 0x81, 2, "\xC5\xB4" },	/* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
	{ 0x82, 2, "\xC5\xB5" },	/* LATIN SMALL LETTER W WITH CIRCUMFLEX */
	{ 0x84, 3, "\xE2\x9C\x98" },	/* HEAVY BALLOT X */
	{ 0x85, 2, "\xC5\xB6" },	/* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
	{ 0x86, 2, "\xC5\xB7" },	/* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
	{ 0x88, 3, "\xE2\x87\x90" },	/* LEFTWARDS DOUBLE ARROW */
	{ 0x89, 3, "\xE2\x87\x92" },	/* RIGHTWARDS DOUBLE ARROW */
	{ 0x8a, 3, "\xE2\x87\x93" },	/* DOWNWARDS DOUBLE ARROW */
	{ 0x8b, 3, "\xE2\x87\x91" },	/* UPWARDS DOUBLE ARROW */
	{ 0x8c, 3, "\xE2\x80\xA6" },	/* HORIZONTAL ELLIPSIS */
	{ 0x8d, 3, "\xE2\x84\xA2" },	/* TRADE MARK SIGN */
	{ 0x8e, 3, "\xE2\x80\xB0" },	/* PER MILLE SIGN */
	{ 0x8f, 3, "\xE2\x80\xA2" },	/* BULLET */
	{ 0x90, 3, "\xE2\x80\x98" },	/* LEFT SINGLE QUOTATION MARK */
	{ 0x91, 3, "\xE2\x80\x99" },	/* RIGHT SINGLE QUOTATION MARK */
	{ 0x92, 3, "\xE2\x80\xB9" },	/* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
	{ 0x93, 3, "\xE2\x80\xBA" },	/* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
	{ 0x94, 3, "\xE2\x80\x9C" },	/* LEFT DOUBLE QUOTATION MARK */
	{ 0x95, 3, "\xE2\x80\x9D" },	/* RIGHT DOUBLE QUOTATION MARK */
	{ 0x96, 3, "\xE2\x80\x9E" },	/* DOUBLE LOW-9 QUOTATION MARK */
	{ 0x97, 3, "\xE2\x80\x93" },	/* EN DASH */
	{ 0x98, 3, "\xE2\x80\x94" },	/* EM DASH */
	{ 0x99, 3, "\xE2\x88\x92" },	/* MINUS SIGN */
	{ 0x9a, 2, "\xC5\x92" },	/* LATIN CAPITAL LIGATURE OE */
	{ 0x9b, 2, "\xC5\x93" },	/* LATIN SMALL LIGATURE OE */
	{ 0x9c, 3, "\xE2\x80\xA0" },	/* DAGGER */
	{ 0x9d, 3, "\xE2\x80\xA1" },	/* DOUBLE DAGGER */
	{ 0x9e, 3, "\xEF\xAC\x81" },	/* LATIN SMALL LIGATURE FI */
	{ 0x9f, 3, "\xEF\xAC\x82" } 	/* LATIN SMALL LIGATURE FL */
};


/**
 * Convert a UTF-8 encoded string into the system local encoding
 *
 * \param string The string to convert
 * \param len The length (in bytes) of the string, or 0
 * \param result Pointer to location in which to store result
 * \return The appropriate utf8_convert_ret value
 */
utf8_convert_ret utf8_to_local_encoding(const char *string, size_t len,
		char **result)
{
	os_error *error;
	int alphabet, i;
	size_t off, prev_off;
	char *temp, *cur_pos;
	const char *enc;
	utf8_convert_ret err;

	assert(string);
	assert(result);

	/* get length, if necessary */
	if (len == 0)
		len = strlen(string);

	/* read system alphabet */
	error = xosbyte1(osbyte_ALPHABET_NUMBER, 127, 0, &alphabet);
	if (error)
		alphabet = territory_ALPHABET_LATIN1;

	/* UTF-8 -> simply copy string */
	if (alphabet == 111 /* UTF-8 */) {
		*result = strndup(string, len);
		return UTF8_CONVERT_OK;
	}

	/* get encoding name */
	enc = (alphabet <= CONT_ENC_END ? localencodings[alphabet - 100]
			      : (alphabet == 120 ?
					localencodings[CONT_ENC_END + 1]
						 : localencodings[0]));

	/* create output buffer */
	*(result) = malloc(len + 1);
	if (!(*result))
		return UTF8_CONVERT_NOMEM;
	*(*result) = '\0';

	prev_off = 0;
	cur_pos = (*result);

	/* Iterate over string, converting input between unconvertable
	 * characters and inserting appropriate output for characters
	 * that iconv can't handle. */
	for (off = 0; off < len; off = utf8_next(string, len, off)) {
		if (string[off] != 0xE2 &&
				string[off] != 0xC5 && string[off] != 0xEF)
			continue;

		for (i = 0; i != NOF_ELEMENTS(special_chars); i++) {
			if (strncmp(string + off, special_chars[i].utf,
					special_chars[i].len) != 0)
				continue;

			/* 0 length has a special meaning to utf8_to_enc */
			if (off - prev_off > 0) {
				err = utf8_to_enc(string + prev_off, enc,
						off - prev_off, &temp);
				if (err != UTF8_CONVERT_OK) {
					assert(err != UTF8_CONVERT_BADENC);
					free(*result);
					return UTF8_CONVERT_NOMEM;
				}

				strcat(cur_pos, temp);

				cur_pos += strlen(temp);

				free(temp);
			}

			*cur_pos = special_chars[i].local;
			*(++cur_pos) = '\0';
			prev_off = off + special_chars[i].len;
		}
	}

	/* handle last chunk
	 * NB. 0 length has a special meaning to utf8_to_enc */

	if (prev_off < len) {
		err = utf8_to_enc(string + prev_off, enc, len - prev_off,
				&temp);
		if (err != UTF8_CONVERT_OK) {
			assert(err != UTF8_CONVERT_BADENC);
			free(*result);
			return UTF8_CONVERT_NOMEM;
		}

		strcat(cur_pos, temp);

		free(temp);
	}

	return UTF8_CONVERT_OK;
}

/**
 * Convert a string encoded in the system local encoding to UTF-8
 *
 * \param string The string to convert
 * \param len The length (in bytes) of the string, or 0
 * \param result Pointer to location in which to store result
 * \return The appropriate utf8_convert_ret value
 */
utf8_convert_ret utf8_from_local_encoding(const char *string, size_t len,
		char **result)
{
	os_error *error;
	int alphabet, i, num_specials = 0, result_alloc;
#define SPECIAL_CHUNK_SIZE 255
	size_t off, prev_off, cur_off;
	char *temp;
	const char *enc;
	utf8_convert_ret err;

	assert(string && result);

	/* get length, if necessary */
	if (len == 0)
		len = strlen(string);

	/* read system alphabet */
	error = xosbyte1(osbyte_ALPHABET_NUMBER, 127, 0, &alphabet);
	if (error)
		alphabet = territory_ALPHABET_LATIN1;

	/* UTF-8 -> simply copy string */
	if (alphabet == 111 /* UTF-8 */) {
		temp = strndup(string, len);
		if (!temp)
			return UTF8_CONVERT_NOMEM;

		*result = temp;
		return UTF8_CONVERT_OK;
	}

	/* get encoding name */
	enc = (alphabet <= CONT_ENC_END ? localencodings[alphabet - 100]
			      : (alphabet == 120 ?
					localencodings[CONT_ENC_END + 1]
						 : localencodings[0]));

	/* create output buffer (oversized) */
	result_alloc = (len * 4) + (3 * SPECIAL_CHUNK_SIZE) + 1;

	*(result) = malloc(result_alloc);
	if (!(*result))
		return UTF8_CONVERT_NOMEM;
	*(*result) = '\0';

	prev_off = 0;
	cur_off = 0;

	/* Iterate over string, converting input between unconvertable
	 * characters and inserting appropriate output for characters
	 * that iconv can't handle. */
	for (off = 0; off < len; off++) {
		if (string[off] < 0x80 || string[off] > 0x9f)
			continue;

		for (i = 0; i != NOF_ELEMENTS(special_chars); i++) {
			if (string[off] != special_chars[i].local)
				continue;

			/* 0 length has a special meaning to utf8_from_enc */
			if (off - prev_off > 0) {
				err = utf8_from_enc(string + prev_off, enc,
						    off - prev_off, &temp, NULL);
				if (err != UTF8_CONVERT_OK) {
					assert(err != UTF8_CONVERT_BADENC);
					LOG(("utf8_from_enc failed"));
					free(*result);
					return UTF8_CONVERT_NOMEM;
				}

				strcat((*result) + cur_off, temp);

				cur_off += strlen(temp);

				free(temp);
			}

			strcat((*result) + cur_off, special_chars[i].utf);

			cur_off += special_chars[i].len;

			prev_off = off + 1;

			num_specials++;
			if (num_specials % SPECIAL_CHUNK_SIZE ==
					SPECIAL_CHUNK_SIZE - 1) {
				char *temp = realloc((*result),
						result_alloc +
						(3 * SPECIAL_CHUNK_SIZE));
				if (!temp) {
					free(*result);
					return UTF8_CONVERT_NOMEM;
				}

				*result = temp;
				result_alloc += (3 * SPECIAL_CHUNK_SIZE);
			}
		}
	}

	/* handle last chunk
	 * NB. 0 length has a special meaning to utf8_from_enc */
	if (prev_off < len) {
		err = utf8_from_enc(string + prev_off, enc, len - prev_off,
				    &temp, NULL);
		if (err != UTF8_CONVERT_OK) {
			assert(err != UTF8_CONVERT_BADENC);
			LOG(("utf8_from_enc failed"));
			free(*result);
			return UTF8_CONVERT_NOMEM;
		}

		strcat((*result) + cur_off, temp);

		cur_off += strlen(temp);

		free(temp);
	}

	/* and copy into more reasonably-sized buffer */
	temp = realloc((*result), cur_off + 1);
	if (!temp) {
		LOG(("realloc failed"));
		free(*result);
		return UTF8_CONVERT_NOMEM;
	}
	*result = temp;

	return UTF8_CONVERT_OK;
}
