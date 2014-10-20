#include "kap.h"

/*********************************************************
 *                  GLOBAL DATA                          *
 *                                                       *
 *********************************************************/
perf_metric_t Puts = {
    .op_base    = { .max = 0.0f,
                    .min = DBL_MAX,
                    .std = 0.0f,
                    .accum = 0.0f,
                    .op_count = 0},
    .throughput = 0.0f,
    .bandwidth  = 0.0f};

perf_metric_t Commit_bn_Puts = {
    .op_base    = { .max = 0.0f,
                    .min = DBL_MAX,
                    .std = 0.0f,
                    .accum = 0.0f,
                    .op_count = 0},
    .throughput = 0.0f,
    .bandwidth  = 0.0f};

perf_metric_t Sync_bn_Puts_Gets = {
    .op_base    = { .max = 0.0f,
                    .min = DBL_MAX,
                    .std = 0.0f,
                    .accum = 0.0f,
                    .op_count = 0},
    .throughput = 0.0f,
    .bandwidth  = 0.0f};

perf_metric_t Gets = {
    .op_base    = { .max = 0.0f,
                    .min = DBL_MAX,
                    .std = 0.0f,
                    .accum = 0.0f,
                    .op_count = 0},
    .throughput = 0.0f,
    .bandwidth  = 0.0f};

double Begin_All = 0.0f;
double End_All = 0.0f;
double Begin_Prod_Phase = 0.0f;
double End_Prod_Phase = 0.0f;
double Begin_Sync_Phase = 0.0f;
double End_Sync_Phase = 0.0f;
double Begin_Cons_Phase = 0.0f;
double End_Cons_Phase = 0.0f;
double Begin = 0.0f;
double End = 0.0f;

/*********************************************************
 *                  PUBLIC FUNCTIONS                     *
 *                                                       *
 *********************************************************/
double
now ()
{
    struct timeval ts;
    double rt;
    gettimeofday (&ts, NULL);
    rt = ((double) (ts.tv_sec)) * 1000000.0;
    rt += (double) ts.tv_usec;
    return rt;
}


void
update_metric (perf_metric_t *m, double b, double e)
{
    double elapse = e - b;
    if (m->op_base.op_count == 0) {
        m->op_base.op_count++;
        m->op_base.M = elapse;
        m->op_base.S = elapse;
        m->op_base.std = 0.0f;
    }
    else {
        /*
         * Running std algorithm using Welford's method.
         */
        double old_M;
        double old_S;
        m->op_base.op_count++;
        old_M = m->op_base.M;
        old_S = m->op_base.S;
        m->op_base.M = old_M
            + ((elapse - old_M)/m->op_base.op_count);
        m->op_base.S = old_S
            + ((elapse - old_M)*(elapse - m->op_base.M));
        m->op_base.std = sqrt ((m->op_base.S)
            / (m->op_base.op_count - 1));
    }

    m->op_base.accum += elapse;

    if (elapse > m->op_base.max)
        m->op_base.max = elapse;
    if (elapse < m->op_base.min)
        m->op_base.min = elapse;
}


