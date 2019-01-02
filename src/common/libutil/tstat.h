/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef _UTIL_TSTAT_H
#define _UTIL_TSTAT_H

typedef struct {
    double min, max;
    int n;
    double M, S, newM, newS;
} tstat_t;

void tstat_push (tstat_t *ts, double x);
double tstat_mean (tstat_t *ts);
double tstat_min (tstat_t *ts);
double tstat_max (tstat_t *ts);
double tstat_variance (tstat_t *ts);
double tstat_stddev (tstat_t *ts);
int tstat_count (tstat_t *ts);

#endif /* !_UTIL_TSTAT_H */
/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
