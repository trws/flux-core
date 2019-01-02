/* Copyright 2014 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#include <unistd.h>

#include "builtin.h"
#include "src/common/libutil/environment.h"

static void print_environment (struct environment *env)
{
    const char *val;
    for (val = environment_first (env); val; val = environment_next (env))
        printf("export %s=\"%s\"\n", environment_cursor (env), val);
    fflush(stdout);
}

static int cmd_env (optparse_t *p, int ac, char *av[])
{
    int n = optparse_option_index (p);
    if (av && av[n]) {
        execvp (av[n], av+n); /* no return if sucessful */
        log_err_exit ("execvp (%s)", av[n]);
    } else {
        struct environment *env = optparse_get_data (p, "env");
        if (env == NULL)
            log_msg_exit ("flux-env: failed to get flux envirnoment!");
        print_environment (env);
    }
    return (0);
}

int subcommand_env_register (optparse_t *p)
{
    optparse_err_t e;
    e = optparse_reg_subcommand (p,
        "env",
        cmd_env,
        "[OPTIONS...] [COMMAND...]",
        "Print the flux environment or execute COMMAND inside it",
        0,
        NULL);
    return (e == OPTPARSE_SUCCESS ? 0 : -1);
}

/*
 * vi: ts=4 sw=4 expandtab
 */
