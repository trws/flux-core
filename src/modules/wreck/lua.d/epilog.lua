--[[Copyright 2014 Lawrence Livermore National Security, LLC
 *  (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  SPDX-License-Identifier: LGPL-3.0
]]

local posix = require 'flux.posix'

-- execute a path from kvs `key`
local function run_kvs (key)
    local epilog = wreck.kvsdir [key] or wreck.flux:kvs_get ("lwj."..key)
    if not epilog then return end
    return os.execute (epilog)
end

function rexecd_complete ()
    local rc, err = run_kvs ("epilog.pre")
    if not rc then wreck:log_msg ("error: epilog: %s", err) end
end

-- rexecd_exit callback happens after the job is in the complete state
function rexecd_exit ()
    local rc, err = run_kvs ("epilog.post")
    if not rc then wreck:log_msg ("error: epilog.post: %s", err) end
end

-- vi: ts=4 sw=4 expandtab
