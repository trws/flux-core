/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */


#ifndef _FLUX_CORE_JOB_H
#define _FLUX_CORE_JOB_H

#include <stdbool.h>
#include <stdint.h>
#include <flux/core.h>

#ifdef __cplusplus
extern "C" {
#endif

enum job_submit_flags {
    FLUX_JOB_PRE_SIGNED = 1,    // 'jobspec' is already signed
};

enum job_priority {
    FLUX_JOB_PRIORITY_MIN = 0,
    FLUX_JOB_PRIORITY_DEFAULT = 16,
    FLUX_JOB_PRIORITY_MAX = 31,
};

enum job_status_flags {
    FLUX_JOB_RESOURCE_REQUESTED     = 1,
    FLUX_JOB_RESOURCE_ALLOCATED     = 2,
    FLUX_JOB_EXEC_REQUESTED         = 4,
    FLUX_JOB_EXEC_RUNNING           = 8,
};

typedef uint64_t flux_jobid_t;

/* Submit a job to the system.
 * 'jobspec' should be RFC 14 jobspec.
 * 'priority' should be a value from 0 to 31 (16 if not instance owner).
 * 'flags' should be 0 for now.
 * The system assigns a jobid and returns it in the response.
 */
flux_future_t *flux_job_submit (flux_t *h, const char *jobspec,
                                int priority, int flags);

/* Parse jobid from response to flux_job_submit() request.
 * Returns 0 on success, -1 on failure with errno set - and an extended
 * error message may be available with flux_future_error_string().
 */
int flux_job_submit_get_id (flux_future_t *f, flux_jobid_t *id);

/* Request a list of active jobs.
 * If 'max_entries' > 0, fetch at most that many jobs.
 * 'json_str' is an encoded JSON array of attribute strings, e.g.
 * ["id","userid",...] that will be returned in response.

 * Process the response payload with flux_rpc_get() or flux_rpc_get_unpack().
 * It is a JSON object containing an array of job objects, e.g.
 * { "jobs":[
 *   {"id":m, "userid":n},
 *   {"id":m, "userid":n},
 *   ...
 * ])
 */
flux_future_t *flux_job_list (flux_t *h, int max_entries,
                              const char *json_str);

/* Remove a job from queue and KVS.
 */
flux_future_t *flux_job_purge (flux_t *h, flux_jobid_t id, int flags);

/* Change job priority.
 */
flux_future_t *flux_job_set_priority (flux_t *h, flux_jobid_t id, int priority);

#ifdef __cplusplus
}
#endif

#endif /* !_FLUX_CORE_JOB_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
