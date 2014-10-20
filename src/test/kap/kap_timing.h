#ifndef __KAP_TIMING_H
#define __KAP_TIMING_H

#define WALL_CLOCK_OUT_FN   "perf-wallclock.out"
#define WALL_CLOCK_DIST_FN  "perf-wallclock.dist"
#define PUTS_OUT_FN         "perf-puts.out"
#define PUTS_DIST_FN        "perf-puts.dist"
#define COMMITS_OUT_FN      "perf-commits.out"
#define COMMITS_DIST_FN     "perf-commits.dist"
#define SYNC_OUT_FN         "perf-sync.out"
#define SYNC_DIST_FN        "perf-sync.dist"
#define GETS_OUT_FN         "perf-gets.out"
#define GETS_DIST_FN        "perf-gets.dist"

typedef struct _perf_base_t {
    double max;
    double min;
    double std;
    double accum;
    double M;
    double S;
    unsigned long op_count; 
} perf_base_t;


typedef struct _perf_metric_t {
    perf_base_t op_base;
    double throughput;
    double bandwidth; 
} perf_metric_t;

extern perf_metric_t Puts;
extern perf_metric_t Commit_bn_Puts;
extern perf_metric_t Sync_bn_Puts_Gets;
extern perf_metric_t Gets;
extern double Begin_All;
extern double End_All;
extern double Begin_Prod_Phase;
extern double End_Prod_Phase;
extern double Begin_Sync_Phase;
extern double End_Sync_Phase;
extern double Begin_Cons_Phase;
extern double End_Cons_Phase;
extern double Begin;
extern double End;

extern double now ();
extern void update_metric (perf_metric_t *m, double b, double e);

#endif /* __KAP_TIMING_H */
