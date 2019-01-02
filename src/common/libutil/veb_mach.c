/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */


#define WORD \
	sizeof(uint)*8

static uint
clz(uint x)
{
	return __builtin_clz(x);
}

static uint
ctz(uint x)
{
	return __builtin_ctz(x);
}

static uint
fls(uint x)
{
	return WORD-clz(x);
}
