--[[Copyright 2014 Lawrence Livermore National Security, LLC
 *  (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  SPDX-License-Identifier: LGPL-3.0
]]

-- Set environment specific to openmpi
--

local dirname = require 'flux.posix'.dirname

function rexecd_init ()
    local env = wreck.environ
    local f = wreck.flux
    local rundir = f:getattr ('broker.rundir')

    -- Avoid shared memory segment name collisions
    -- when flux instance runs >1 broker per node.
    env['OMPI_MCA_orte_tmpdir_base'] = rundir
end

-- vi: ts=4 sw=4 expandtab
