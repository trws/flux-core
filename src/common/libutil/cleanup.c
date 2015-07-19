/*****************************************************************************\
 *  Copyright (c) 2014 Lawrence Livermore National Security, LLC.  Produced at
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

#include "cleanup.h"
#include "xzmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <czmq.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct cleaner {
    cleaner_fun_f * fun;
    void * arg;
};

void cleanup_directory (const struct cleaner *c)
{
    if(c && c->arg)
        rmdir(c->arg);
}

void cleanup_file (const struct cleaner *c)
{
    if(c && c->arg)
        unlink(c->arg);
}

static pid_t cleaner_pid = 0;
static zlist_t *cleanup_list = NULL;
static void cleanup (void)
{
    pthread_mutex_lock(&mutex);
    if ( ! cleanup_list || cleaner_pid != getpid()) return;
    const struct cleaner * c = zlist_first(cleanup_list);
    while(c){
        /* fprintf(stderr, "cleaning up %s in %d\n", (char*)c->arg, getpid()); */
        if (c && c->fun){
            c->fun(c);
        }
        c = zlist_next(cleanup_list);
    }
    zlist_destroy(&cleanup_list);
    cleanup_list = NULL;
    pthread_mutex_unlock(&mutex);
}

void cleanup_push (cleaner_fun_f *fun, void * arg)
{
    pthread_mutex_lock(&mutex);
    /* fprintf(stderr, "prepending cleanup for %s in %d\n", (char*)arg, getpid()); */
    if (! cleanup_list || cleaner_pid != getpid())
    {
        // This odd dance is to handle forked processes that do not exec
        if (cleaner_pid != 0 && cleanup_list) {
            zlist_destroy(&cleanup_list);
        }
        cleanup_list = zlist_new();
        cleaner_pid = getpid();
        atexit(cleanup);
    }
    struct cleaner * c = calloc(sizeof(struct cleaner), 1);
    c->fun = fun;
    c->arg = arg;
    zlist_push(cleanup_list, c);
    pthread_mutex_unlock(&mutex);
}

void cleanup_push_string (cleaner_fun_f *fun, const char * path)
{
    cleanup_push(fun, xstrdup(path));
}
