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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <flux/core.h>

int mod_main (flux_t h, zhash_t *args_in)
{
    zhash_t *args = zhash_dup(args_in);
    Py_SetProgramName("pymod");
    Py_Initialize();

    PyObject *search_path = PySys_GetObject("path");

    //TODO: make this use the flux module search path to find trampoline
    PyList_Append(search_path, PyString_FromString("/g/g12/scogland/projects/flux/flux-core/src/modules/pymod"));
    char * module_path = zhash_lookup(args, "--path");
    if(module_path){
        PyList_Append(search_path, PyString_FromString(module_path));
    }
    PySys_SetObject("path", search_path);

    char * module_name = zhash_lookup(args, "--module");
    if(!module_name){
        flux_log(h, LOG_ERR, "Module name must be specified with --module");
        return EINVAL;
    }
    zhash_delete(args, "--module");
    flux_log(h, LOG_ERR, "loading module named: %s\n", module_name);

    PyObject *trampoline_module_name = PyString_FromString("fluxmod_ctypes");
    PyObject *module = PyImport_Import(trampoline_module_name);
    if(!module){
        PyErr_Print();
        return EINVAL;
    }
    Py_DECREF(trampoline_module_name);

    if(module){
        PyObject *mod_main = PyObject_GetAttrString(module, "mod_main_trampoline");
        if(mod_main && PyCallable_Check(mod_main)){
            //maybe unpack args directly? probably easier to use a dict
            PyObject *py_args = PyTuple_New(3);
            PyTuple_SetItem(py_args, 0, PyString_FromString(module_name));
            PyTuple_SetItem(py_args, 1, PyCapsule_New(h, "flux-handle", NULL));

            //Convert zhash to native python dict, should preserve mods
            //through switch to argc-style arguments
            PyObject *arg_dict = PyDict_New();
            void * value = zhash_first(args);
            const char * key = zhash_cursor(args);
            for(;value != NULL; value = zhash_next(args), key = zhash_cursor(args)){
                PyDict_SetItemString(arg_dict, key, PyString_FromString(value));
            }

            PyTuple_SetItem(py_args, 2, arg_dict);
            // Call into trampoline
            PyObject_CallObject(mod_main, py_args);
            if(PyErr_Occurred()){
                PyErr_Print();
            }
            Py_DECREF(py_args);
            Py_DECREF(arg_dict);
        }
    }

    /* old test code, remove before pushing in */
    /* PyRun_SimpleString( "from time import time,ctime\n" */
    /*         "import sys\n" */
    /*         "print sys.path\n" */
    /*         "print 'Today is',ctime(time())\n"); */
    Py_Finalize();
    zhash_destroy(&args);
    return 0;
}

MOD_NAME ("pymod");

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
