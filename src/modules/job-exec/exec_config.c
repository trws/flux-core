/************************************************************\
 * Copyright 2021 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* Flux bulk-exec configuration code */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <jansson.h>
#include <unistd.h>

#if HAVE_FLUX_SECURITY
#include <flux/security/version.h>
#endif

#include "exec_config.h"
#include "ccan/str/str.h"
#include "src/common/libutil/errno_safe.h"

static const char *default_cwd = "/tmp";

struct exec_config {
    const char *default_job_shell;
    const char *flux_imp_path;
    const char *exec_service;
    int exec_service_override;
    json_t *sdexec_properties;
};

/* Global configs initialized in config_init() */
static struct exec_config exec_conf;

static const char *jobspec_get_job_shell (json_t *jobspec)
{
    const char *path = NULL;
    if (jobspec)
        (void) json_unpack (jobspec,
                            "{s:{s:{s:{s:s}}}}",
                            "attributes",
                             "system",
                              "exec",
                               "job_shell", &path);
    return path;
}

const char *config_get_job_shell (struct jobinfo *job)
{
    const char *path = NULL;
    if (job)
        path = jobspec_get_job_shell (job->jobspec);
    return path ? path : exec_conf.default_job_shell;
}

static const char *jobspec_get_cwd (json_t *jobspec)
{
    const char *cwd = NULL;
    if (jobspec)
        (void) json_unpack (jobspec,
                            "{s:{s:{s:s}}}",
                             "attributes",
                              "system",
                               "cwd", &cwd);
    return cwd;
}

const char *config_get_cwd (struct jobinfo *job)
{
    const char *cwd = NULL;
    if (job) {
        if (job->multiuser)
            cwd = "/";
        else if (!(cwd = jobspec_get_cwd (job->jobspec)))
            cwd = default_cwd;
    }
    return cwd;
}

const char *config_get_imp_path (void)
{
    return exec_conf.flux_imp_path;
}

const char *config_get_exec_service (void)
{
    return exec_conf.exec_service;
}

bool config_get_exec_service_override (void)
{
    return exec_conf.exec_service_override;
}

json_t *config_get_sdexec_properties (void)
{
    return exec_conf.sdexec_properties;
}

static int config_add_stats_string (json_t *o,
                                    const char *key,
                                    const char *value)
{
    json_t *s;

    if (!value)
        return 0;

    if (!(s = json_string (value))) {
        errno = ENOMEM;
        return -1;
    }
    if (json_object_set_new (o, key, s) < 0) {
        json_decref (s);
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

static int config_add_stats_int (json_t *o,
                                 const char *key,
                                 int value)
{
    json_t *s = json_integer (value);
    if (!s) {
        errno = ENOMEM;
        return -1;
    }
    if (json_object_set_new (o, key, s) < 0) {
        json_decref (s);
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

int config_get_stats (json_t **config_stats)
{
    json_t *o = NULL;

    if (!(o = json_object ())) {
        errno = ENOMEM;
        goto error;
    }

    if (config_add_stats_string (o,
                                 "default_cwd",
                                 default_cwd) < 0
        || config_add_stats_string (o,
                                    "default_job_shell",
                                    exec_conf.default_job_shell) < 0
        || config_add_stats_string (o,
                                    "flux_imp_path",
                                    exec_conf.flux_imp_path) < 0
        || config_add_stats_string (o,
                                    "exec_service",
                                    exec_conf.exec_service) < 0)
        goto error;

    if (config_add_stats_int (o,
                              "exec_service_override",
                              exec_conf.exec_service_override) < 0)
        goto error;

    if (exec_conf.sdexec_properties) {
        if (json_object_set (o,
                             "sdexec_properties",
                             exec_conf.sdexec_properties) < 0)
            goto error;
    }

    (*config_stats) = o;
    return 0;

error:
    ERRNO_SAFE_WRAP (json_decref, o);
    return -1;
}

static void exec_config_init (struct exec_config *ec)
{
    ec->default_job_shell = flux_conf_builtin_get ("shell_path", FLUX_CONF_AUTO);
    ec->flux_imp_path = NULL;
    ec->exec_service = "rexec";
    ec->exec_service_override = 0;
    ec->sdexec_properties = NULL;
}

/*  Initialize configurations for use by job-exec bulk-exec
 *  implementation
 */
int config_init (flux_t *h, int argc, char **argv)
{
    flux_error_t err;

    /* Per trws comment in 97421e88987535260b10d6a19551cea625f26ce4
     *
     * The musl libc loader doesn't actually unload objects on
     * dlclose, so a subsequent dlopen doesn't re-clear globals and
     * similar.
     *
     * So we must re-initialize globals on each config_init().
     */
    exec_config_init (&exec_conf);

    /*  Check configuration for exec.job-shell */
    if (flux_conf_unpack (flux_get_conf (h),
                          &err,
                          "{s?{s?s}}",
                          "exec",
                            "job-shell", &exec_conf.default_job_shell) < 0) {
        flux_log (h, LOG_ERR,
                  "error reading config value exec.job-shell: %s",
                  err.text);
        return -1;
    }

    /*  Check configuration for exec.imp */
    if (flux_conf_unpack (flux_get_conf (h),
                          &err,
                          "{s?{s?s}}",
                          "exec",
                            "imp", &exec_conf.flux_imp_path) < 0) {
        flux_log (h, LOG_ERR,
                  "error reading config value exec.imp: %s",
                  err.text);
        return -1;
    }

    /*  Check configuration for exec.service and exec.service-override */
    if (flux_conf_unpack (flux_get_conf (h),
                          &err,
                          "{s?{s?s s?b}}",
                          "exec",
                            "service", &exec_conf.exec_service,
                            "service-override",
                              &exec_conf.exec_service_override) < 0) {
        flux_log (h, LOG_ERR,
                  "error reading config value exec.service: %s",
                  err.text);
        return -1;
    }

    /*  Check configuration for exec.sdexec-properties */
    if (flux_conf_unpack (flux_get_conf (h),
                          &err,
                          "{s?{s?o}}",
                          "exec",
                            "sdexec-properties",
                              &exec_conf.sdexec_properties) < 0) {
        flux_log (h, LOG_ERR,
                  "error reading config table exec.sdexec-properties: %s",
                  err.text);
        return -1;
    }
    if (exec_conf.sdexec_properties) {
        const char *k;
        json_t *v;

        if (!json_is_object (exec_conf.sdexec_properties)) {
            flux_log (h, LOG_ERR, "exec.sdexec-properties is not a table");
            errno = EINVAL;
            return -1;
        }
        json_object_foreach (exec_conf.sdexec_properties, k, v) {
            if (!json_is_string (v)) {
                flux_log (h, LOG_ERR,
                          "exec.sdexec-properties.%s is not a string",
                          k);
                errno = EINVAL;
                return -1;
            }
        }
    }

    if (argv && argc) {
        /* Finally, override values on cmdline */
        for (int i = 0; i < argc; i++) {
            if (strstarts (argv[i], "job-shell="))
                exec_conf.default_job_shell = argv[i]+10;
            else if (strstarts (argv[i], "imp="))
                exec_conf.flux_imp_path = argv[i]+4;
            else if (strstarts (argv[i], "service="))
                exec_conf.exec_service = argv[i]+8;
        }
    }

    return 0;
}

/* vi: ts=4 sw=4 expandtab
 */
