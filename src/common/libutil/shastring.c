/*****************************************************************************\
 *  Copyright (c) 2015 Lawrence Livermore National Security, LLC.  Produced at
 *  the Lawrence Livermore National Laboratory (cf, AUTHORS, DISCLAIMER.LLNS).
 *  LLNL-CODE-658032 All rights reserved.
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  Flux is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also:  http://www.gnu.org/licenses/
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <ctype.h>
#include <string.h>
#include "shastring.h"

static uint8_t xtoint (char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    return c - 'A' + 0xA;
}

static char inttox (uint8_t i)
{
    if (i <= 9)
        return '0' + i;
    return 'A' + i - 0xa;
}

int sha1_strtohash (const char *s, uint8_t hash[20])
{
    int i;
    if (strlen (s) != 40)
        return -1;
    for (i = 0; i < 20; i++) {
        if (!isxdigit (s[i*2]) || !isxdigit (s[i*2 + 1]))
            return -1;
        hash[i] = xtoint (s[i*2]) << 4;
        hash[i] |= xtoint (s[i*2 + 1]);
    }
    return 0;
}


void sha1_hashtostr (const uint8_t hash[20], char s[41])
{
    int i;
    for (i = 0; i < 20; i++) {
        s[i*2] = inttox (hash[i] >> 4);
        s[i*2 + 1] = inttox (hash[i] & 0xf);
    }
    s[i*2] = '\0';
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
