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
#include <unistd.h>

#include <czmq.h>

struct cleaner {
    cleaner_fun_t * fun;
    char * path;
};

void clean_directory (const struct cleaner *c)
{
    if(c && c->path)
        rmdir(c->path);
}

void clean_file (const struct cleaner *c)
{
    if(c && c->path)
        unlink(c->path);
}

static zlist_t *cleanup_list = NULL;
static void cleanup (void)
{
    if ( ! cleanup_list ) return;
    const struct cleaner * c = zlist_first(cleanup_list);
    while(c){
        if (c && c->fun){
            c->fun(c);
        }
        c = zlist_next(cleanup_list);
    }
}

void add_cleaner (cleaner_fun_t *fun, const char * path)
{
    if (! cleanup_list)
    {
        cleanup_list = zlist_new();
        atexit(cleanup);
    }
    struct cleaner * c = calloc(sizeof(struct cleaner), 1);
    c->fun = fun;
    c->path = xstrdup(path);
    zlist_push(cleanup_list, c);
}
