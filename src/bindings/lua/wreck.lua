--[[Copyright 2014 Lawrence Livermore National Security, LLC
 *  (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 *  This file is part of the Flux resource manager framework.
 *  For details, see https://github.com/flux-framework.
 *
 *  SPDX-License-Identifier: LGPL-3.0
]]

local posix = require 'flux.posix'
local hostlist = require 'flux.hostlist'
local cpuset = require 'flux.affinity'.cpuset

local wreck = {}
wreck.__index = wreck;

local lwj_options = {
--    ['ntasks'] =                "Set total number of tasks to execute",
    ['commit-on-task-exit'] =   "Call kvs_commit for each task exit",
    ['stdio-delay-commit'] =    "Don't call kvs_commit for each line of output",
    ['stdio-commit-on-open'] =  "Commit to kvs on stdio open in each task",
    ['stdio-commit-on-close'] = "Commit to kvs on stdio close in each task",
    ['nokz'] =                  "Do not store job output in kvs",
    ['stop-children-in-exec'] = "Start tasks in STOPPED state for debugger",
    ['no-pmi-server'] =         "Do not start simple-pmi server",
    ['trace-pmi-server'] =      "Log simple-pmi server protocol exchange",
    ['cpu-affinity'] =          "Set default cpu-affinity to assigned cores",
    ['gpubind'] =               "Control CUDA_VISIBLE_DEVICES [=per-task,off]",
    ['mpi'] =                   "Set hint for type of MPI, e.g. -o mpi=spectrum "
}

local default_opts = {
    ['help']    = { char = 'h'  },
    ['name']    = { char = 'J', arg = "NAME" },
    ['verbose'] = { char = 'v'  },
    ['ntasks']  = { char = 'n', arg = "N" },
    ['gpus-per-task']  = { char = 'g', arg = "g" },
    ['cores-per-task']  = { char = 'c', arg = "N" },
    ['nnodes']  = { char = 'N', arg = "N" },
    ['tasks-per-node']  =
                   { char = 't', arg = "N" },
    ['walltime'] = { char = "T", arg = "SECONDS" },
    ['output'] =   { char = "O", arg = "FILENAME" },
    ['error'] =    { char = "E", arg = "FILENAME" },
    ['input'] =    { char = "i", arg = "HOW" },
    ['label-io'] = { char = "l", },
    ['skip-env'] = { char = "S", },
    ['epilog']  =  { char = "x", arg = "SCRIPT" },
    ['postscript'] =
                   { char = "p", arg = "SCRIPT" },
    ['options'] = { char = 'o', arg = "OPTIONS.." },
}

local function opt_table (w)
    local o = {}
    for x,t in pairs (default_opts) do
        o[x] = t.char
    end
    for _,v in pairs (w.extra_options) do
        o[v.name] = v.char
    end
    return o
end

local function short_opts (w)
    local s = ""
    for x,t in pairs (default_opts) do
        s = s .. t.char .. (t.arg and ":" or "")
    end
    for _,v in pairs (w.extra_options) do
        s = s .. v.char .. (v.arg and ":" or "")
    end
    return s
end

function wreck:say (...)
    io.stderr:write (self.prog..": "..string.format (...))
end

function wreck:verbose (...)
    if self.opts.v then
        self:say (...)
    end
end

function wreck:die (...)
    self:say (...)
    os.exit (1)
end

function wreck:usage()
    io.stderr:write ("Usage: "..self.prog.." OPTIONS.. COMMANDS..\n")
    io.stderr:write ([[
  -h, --help                 Display this message
  -v, --verbose              Be verbose
  -N, --name=NAME            Set an optional name for job to NAME
  -n, --ntasks=N             Request to run a total of N tasks
  -c, --cores-per-task=N     Request N cores per task
  -g, --gpus-per-task=N      Request N GPUs per task
  -N, --nnodes=N             Force number of nodes
  -t, --tasks-per-node=N     Force number of tasks per node
  -o, --options=OPTION,...   Set other options (See OTHER OPTIONS below)
  -T, --walltime=N[SUFFIX]   Set max job walltime to N seconds. Optional
                             suffix may be 's' for seconds (default), 'm'
                             for minutes, 'h' for hours or 'd' for days.
                             N may be an arbitrary floating point number,
                             but will be rounded up to nearest second.
  -O, --output=FILENAME      Duplicate stdout/stderr from tasks to a file or
                             files. FILENAME is optionally a mustache
                             template with keys such as id, cmd, taskid.
                             (e.g. --output=flux-{{id}}.out)
  -i, --input=HOW            Indicate how to deal with stdin for tasks. HOW
                             is a list of [src:]dst pairs separated by semi-
                             colon, where src is an optional src file
                             (default: stdin) and dst is a list of taskids
                             or "*" or "all". E.g. (-i0 sends stdin to task 0,
                             -i /dev/null:* closes all stdin, etc.)
  -E, --error=FILENAME       Send stderr to a different location than stdout.
  -l, --labelio              Prefix lines of output with task id
  -S, --skip-env             Skip export of environment to job
  -x, --epilog=PATH          Execute a script after all tasks exit but before
                             the job state is set to "complete"
  -p, --postscript=PATH      Execute a script after job state is "complete"
]])
    for _,v in pairs (self.extra_options) do
        local optstr = v.name .. (v.arg and "="..v.arg or "")
        io.stderr:write (
            string.format ("  -%s, --%-20s %s\n", v.char, optstr, v.usage))
    end
    io.stderr:write ("\nOTHER OPTIONS:\n")
    for o,d in pairs (lwj_options) do
        io.stderr:write (string.format ("  %-26s %s\n", o, d))
    end
end


function wreck.get_filtered_env ()
    local env = posix.getenv()
    env.HOSTNAME = nil
    env.ENVIRONMENT = nil
    for k,v in pairs (env) do
        if k:match ("SLURM_") then env[k] = nil end
        if k:match ("FLUX_URI") then env[k] = nil end
    end
    return (env)
end

local function get_job_env (arg)
    local f = arg.flux
    local env = wreck.get_filtered_env ()
    local default_env = {}
    if f then default_env = f:kvs_get ("lwj.environ") or {} end
    for k,v in pairs (env) do
        -- If same key=value is already in default env no need to
        --  export it again, remove:
        if default_env[k] == env[k] then env[k] = nil end
    end
    return (env)
end

local function array_tonumber (t)
    for i = 1, #t do
        t[i] = tonumber (t[i])
    end
    return t
end


local function job_kvspath (f, id)
    assert (id, "Required argument id missing!")
    local arg = { id }
    if type (id) == "table" then
        arg = id
    end
    local r, err = f:rpc ("job.kvspath", {ids = array_tonumber (arg) })
    if not r then error (err) end
    if type (id) == "table" then return r.paths end
    return r.paths [1]
end

---
-- Return kvs path to job id `id`
--
local kvs_paths = {}
local function kvs_path_insert (id, path)
    kvs_paths [id] = path
end
local function kvs_path (f, id)
    if not kvs_paths[id] then
        kvs_path_insert (id, job_kvspath (f, id))
    end
    return kvs_paths [id]
end

local function kvs_path_multi (f, ids)
    local result = job_kvspath (f, ids)
    for i,id in ipairs (ids) do
        kvs_path_insert (id, result [id])
    end
    return result
end

function wreck:lwj_path (id)
    assert (id, "missing required argument `id'")
    if not self.lwj_paths[id] then
        self.lwj_paths [id] = job_kvspath (self.flux, id)
    end
    return self.lwj_paths [id]
end

function wreck:kvsdir (id)
    if not self.kvsdirs[id] then
        local f = self.flux
        local d, err = f:kvsdir (self:lwj_path (id))
        if not d then return nil, err end
        self.kvsdirs[id] = d
    end
    return self.kvsdirs[id]
end

function wreck:add_options (opts)
    for k,v in pairs (opts) do
        if not type (v) == "table" then return nil, "Invalid parameter" end

        local char = v.char
        if default_opts [v.name] then
            return nil, "Can't override option '"..k.."'"
        end
        table.insert (self.extra_options, v)
    end
    return true
end

function wreck:getopt (opt)
    if not self.opts then return nil  end
    return self.opts [opt]
end

local function parse_walltime (s)
    local m = { s = 1, m = 60, h = 3600, d = 56400 }
    local n, suffix = s:match ("^([0-9.]+)([HhMmDdSs]?)$")
    if not tonumber (n) then
        return nil, "Invalid duration '"..s.."'"
    end
    if suffix and m [suffix]  then
        n = (n * m [suffix])
    end
    return math.ceil (n)
end

function wreck:initial_job_options ()
    if not self.flux then
        local err
        self.flux, err = require 'flux'.new ()
        if not self.flux then
            self:die ("Unable to open connection to flux broker: %s", err)
        end
    end
    return self.flux:kvs_get ("lwj.options") or {}
end

-- Create output configuration table from wreck object state,
function wreck:output_file_config ()
    if not self.opts.O and not self.opts.E then return nil end
    return {
        files = {
            stdout = self.opts.O,
            stderr = self.opts.E,
        },
        labelio = self.opts.l,
    }
end

function wreck:parse_cmdline (arg)
    local getopt = require 'flux.alt_getopt' .get_opts
    local s = short_opts (self)
    local v = opt_table (self)
    self.opts, self.optind = getopt (arg, s, v)

    if self.opts.h then
        self:usage()
        os.exit (0)
    end

    if self.opts.o then
        for opt in self.opts.o:gmatch ("[^,]+") do
            if not lwj_options [opt:match ("([^=]+)")] then
                return nil, string.format ("Unknown LWJ option '%s'\n", opt)
            end
        end
    end

    if self.opts.T then
        self.walltime, err = parse_walltime (self.opts.T)
        if not self.walltime then
            self:die ("Error: %s", err)
        end
    end

    if self.optind > #arg then
        self:say ("Error: remote command required\n")
        self:usage()
        os.exit (1)
    end

    self.nnodes = self.opts.N and tonumber (self.opts.N)

    if not self.opts.t then
        self.opts.t = 1
    end

    -- If nnodes was provided but -n, --ntasks not set, then
    --  set ntasks to nnodes.
    if self.opts.N and not self.opts.n then
        self.ntasks = self.nnodes * self.opts.t
    else
        self.ntasks = self.opts.n and tonumber (self.opts.n) or 1
    end
    if self.opts.c then
        self.ncores = self.opts.c * self.ntasks
    else
        self.ncores = self.ntasks
    end

    if self.opts.g then
        self.ngpus = self.opts.g * self.ntasks
    end

    self.tasks_per_node = self.opts.t

    self.cmdline = {}
    for i = self.optind, #arg do
        table.insert (self.cmdline, arg[i])
    end

    if not self.flux then
        self.flux = require 'flux'.new()
    end

    self.job_options = self:initial_job_options ()
    if self.opts.o then
        for entry in self.opts.o:gmatch ('[^,]+') do
            local opt, val = entry, 1
            if entry:match ("=") then
                opt, val = entry:match ("(.+)=(.+)")
                if tonumber(val) == 0 or val == "false" or val == "no" then
                    val = false
                end
            end
            self.job_options[opt] = val
        end
    end

    self.output = self:output_file_config ()
    return true
end

local function inputs_table_from_args (wreck, s)
    -- Default is to broadcast stdin to all tasks:
    if not s then return { { src = "stdin", dst = "*" } } end

    local inputs = {}
    for m in s:gmatch ("[^;]+") do
        local src,dst = m:match ("^([^:]-):?([^:]+)$")
        if not src or not dst then
            wreck:die ("Invalid argument to --input: %s\n", s)
        end
        --  A dst of "none" short circuits rest of entries
        if dst == "none" then return inputs end

        --  Verify the validity of dst
        if dst ~= "all" and dst ~= "*" then
            local h, err = cpuset.new (dst)
            if not h then
                if src ~= "" then
                    wreck:die ("--input: invalid dst: %s\n", dst)
                end
                -- otherwise, assume only dst was provided
                src = dst
                dst = "*"
            end
        end
        if src == "" or src == "-" then src = "stdin" end
        table.insert (inputs, { src = src, dst = dst })
    end
    -- Add fall through option
    table.insert (inputs, { src = "/dev/null", dst = "*" })
    return inputs
end

local function fixup_nnodes (wreck)
    if not wreck.flux then return end
    if not wreck.nnodes then
        wreck.nnodes = math.min (wreck.ntasks, wreck.flux.size)
    elseif wreck.nnodes > wreck.flux.size then
        wreck:die ("Requested nodes (%d) exceeds available (%d)\n",
                  wreck.nnodes, wreck.flux.size)
    end
end

local function random_string ()
    local f = assert (io.open ("/dev/urandom", "rb"))
    local d = f:read(8)
    f:close()
    local s = ""
    for i = 1, d:len() do
        s = s..string.format ("%02x", d:byte(i))
    end
    return (s)
end

function wreck:setup_ioservices ()
    -- Setup stderr/stdout ioservice ranks. Either flux-wreckrun
    --  or nodeid 0 of the job will offer one or both of these services:
    self.ioservices = {}
    if self.job_options.nokz then
        -- Default: register stdout and stderr service on this rank
        local name = random_string ()
        self.ioservices = {
            stdout = {
                name = name..":out",
                rank = self.flux.rank
            },
            stderr = {
                name = name..":err",
                rank = self.flux.rank
            }
        }
        if self.output then
            -- Shunt stdio to nodeid 0 of the job if any output is being
            -- redirected to files via the output.lua plugin:
            local FLUX_NODEID_ANY = require 'flux'.NODEID_ANY
            local outfile = self.output.files.stdout
            local errfile = self.output.files.stderr
            if outfile then
                self.ioservices.stdout.rank = FLUX_NODEID_ANY
            end
            if errfile or outfile then
                self.ioservices.stderr.rank = FLUX_NODEID_ANY
            end
        end
    end
    return self.ioservices
end


function wreck:jobreq ()
    if not self.opts then return nil, "Error: cmdline not parsed" end
    if self.fixup_nnodes then
        fixup_nnodes (self)
    end
    local jobreq = {
        name   =  self.opts.J,
        nnodes =  self.nnodes or 0,
        ntasks =  self.ntasks,
        ncores =  self.ncores,
        cmdline = self.cmdline,
        environ = self.opts.S and {} or get_job_env { flux = self.flux },
        cwd =     posix.getcwd (),
        walltime =self.walltime or 0,
        ngpus = self.ngpus or 0,

        ["opts.nnodes"] = self.opts.N,
        ["opts.ntasks"]  = self.opts.n,
        ["opts.cores-per-task"] = self.opts.c,
        ["opts.gpus-per-task"] = self.opts.g,
        ["opts.tasks-per-node"] = self.opts.t,
        ["epilog.pre"] = self.opts.x,
        ["epilog.post"] = self.opts.p,
    }
    jobreq.options = self.job_options
    jobreq.output = self.output
    jobreq ["input.config"] = inputs_table_from_args (self, self.opts.i)

    jobreq.ioservice = self:setup_ioservices ()

    return jobreq
end

local function send_job_request (self, name)
    if not self.flux then
        local f, err = require 'flux'.new ()
        if not f then return nil, err end
        self.flux = f
    end
    local jobreq, err = self:jobreq ()
    if not jobreq then
        return nil, err
    end
    local resp, err = self.flux:rpc (name, jobreq)
    if not resp then
        return nil, "flux.rpc: "..err
    end
    if resp.errnum then
        return nil, name.." message failed with errnum="..resp.errnum
    end
    return resp
end

function wreck:submit ()
    local resp, err = send_job_request (self, "job.submit")
    if not resp then return nil, err end
    if resp.state ~= "submitted" then
        return nil, "job submission failure, job left in state="..resp.state
    end
    return resp.jobid, resp.kvs_path
end

function wreck:createjob ()
    self.fixup_nnodes = true
    local resp, err = send_job_request (self, "job.create")
    if not resp then return nil, err end
    --
    -- If reply contains a kvs path to this LWJ, pre-memoize the id
    --  to kvs path mapping so we do not have to fetch it again.
    if resp.kvs_path then
        self.lwj_paths [resp.jobid] = resp.kvs_path
        kvs_paths [resp.jobid] = resp.kvs_path
    end
    return resp.jobid, resp.kvs_path
end

local function initialize_args (arg)
    if arg.ntasks and arg.nnodes then return true end
    local f = arg.flux
    local lwj, err = f:kvsdir (kvs_path (f, arg.jobid))
    if not lwj then
        return nil, "Error: "..err
    end
    arg.ntasks = lwj.ntasks
    arg.nnodes = lwj.nnodes
    return true
end

function wreck.ioattach (arg)
    local ioplex = require 'wreck.io'
    local f = arg.flux
    local rc, err = initialize_args (arg)
    if not rc then return nil, err end

    local taskio, err = ioplex.create (arg)
    if not taskio then return nil, err end
    for i = 0, arg.ntasks - 1 do
        for _,stream in pairs {"stdout", "stderr"} do
            local rc, err =  taskio:redirect (i, stream, stream)
        end
    end
    taskio:start (arg.flux)
    return taskio
end

local logstream = {}
logstream.__index = logstream

local function logstream_print_line (iow, line)
    io.stderr:write (string.format ("rank%d: Error: %s\n", iow.id, line))
end

function logstream:dump ()
    for _, iow in pairs (self.watchers) do
        local r, err = iow.kz:read ()
        while r and r.data and not r.eof do
           logstream_print_line (iow, r.data)
           r, err = iow.kz:read ()
        end
    end
end

function wreck.logstream (arg)
    local l = {}
    local f = arg.flux
    if not f then return nil, "flux argument member required" end
    local rc, err = initialize_args (arg)
    if not rc then return nil, err end
    if not arg.nnodes then return nil, "nnodes argument required" end
    l.watchers = {}
    for i = 0, arg.nnodes - 1 do
        local key = kvs_path (f, arg.jobid)..".log."..i
        local iow, err = f:iowatcher {
            key = key,
            kz_flags = arg.oneshot and { "nofollow" },
            handler = function (iow, r, err)
                if not r then
                    if err then
                        io.stderr:write ("logstream kz error "..err.."\n")
                    end
                    return
                end
                logstream_print_line (iow, r)
            end
        }
        iow.id = i
        table.insert (l.watchers, iow)
    end
    return setmetatable (l, logstream)
end

local function exit_message (t)
    local s = "exited with "
    s = s .. (t.exit_code and "exit code" or "signal") .. " %d"
    return s:format (t.exit_code or t.exit_sig)
end

local function task_status (lwj, taskid)
    if not tonumber (taskid) then return nil end
    local t = lwj[taskid]
    if not t.exit_status then
        return 0, "running"
    end
    local x = t.exit_code or (t.exit_sig + 128)
    return x, exit_message (t)
end

local function exit_status_aggregate (arg)
    local flux = require 'flux'
    local f = arg.flux
    local lwj = arg.kvsdir
    local msgs = {}

    local exit_status = lwj.exit_status
    if not exit_status then return nil, nil end

    local s, max, core = flux.exitstatus (exit_status.max)
    if s == "killed" then max = max + 128 end

    for ids,v in pairs (exit_status.entries) do
        local msg, code, core = flux.exitstatus (v)
        s = "exited with "
        s = s .. (msg == "exited" and "exit code " or "signal ") .. code
        msgs [s] = hostlist.new (ids)
    end

    return max, msgs
end


--- Return highest exit code from all tasks and task exit message list.
-- Summarize job exit status for arg.jobid by returning max task exit code,
-- along with a list of task exit messages to be optionally emitted to stdout.
-- @param arg.jobid job identifier
-- @param arg.flux  (optional) flux handle
-- @return exit_cde, msg_list
function wreck.status (arg)
    local flux = require 'flux'
    local f = arg.flux
    local jobid = arg.jobid
    if not jobid then return nil, "required arg jobid" end

    if not f then f = require 'flux'.new() end
    local lwj = f:kvsdir (kvs_path (f, jobid))

    local max = 0
    local msgs = {}

    if lwj.exit_status then
        return exit_status_aggregate { flux = f, kvsdir = lwj }
    end

    for taskid in lwj:keys () do
        local x, s = task_status (lwj, taskid)
        if x then
            if x > max then max = x end
            if not msgs[s] then
                msgs[s] = hostlist.new (taskid)
            else
                msgs[s]:concat (taskid)
            end
        end
    end
    return max, msgs
end

function wreck.id_to_path (arg)
    local flux = require 'flux'
    local f = arg.flux
    local id = arg.jobid
    if not id then return nil, "required arg jobid" end
    if not f then f, err = require 'flux'.new () end
    if not f then return nil, err end
    return kvs_path (f, id)
end

local function kvsdir_reverse_keys (dir)
    local result = {}
    for k in dir:keys () do
        if tonumber (k) then
            table.insert (result, k)
        end
    end
    table.sort (result, function (a,b) return tonumber(b) < tonumber(a) end)
    return result
end

local function reverse (t)
    local len = #t
    local r = {}
    for i = len, 1, -1 do
        table.insert (r, t[i])
    end
    return r
end

-- append array t2 to t1
local function append (t1, t2)
    for _,v in ipairs (t2) do table.insert (t1, v) end
    return t1
end

function wreck.jobids_to_kvspath (arg)
    local f = arg.flux
    local ids = arg.jobids
    if not ids then return nil, 'missing required arg jobids' end
    if not f then f, err = require 'flux'.new () end
    if not f then return nil, err end
    return kvs_path_multi (f, ids)
end

local function include_state (conf, state)
    -- Include all states if no state conf is supplied
    if not conf then return true end

    -- Otherwise, if there is a conf.include hash then *only* include
    --  states that appear in this hash:
    if conf.include then return conf.include[state] end

    -- Otherwise, exclude this state if it is in the conf.exclude hash:
    if conf.exclude then return not conf.exclude[state] end

    -- No conf.include, no conf.exclude, I guess it is included:
    return true
end

-- Convert keys of table t to a comma-separated list of strings
local function to_string_list (t)
    if not t then return nil end
    local r = {}
    for k,v in pairs (t) do
        if v then table.insert (r, k) end
    end
    return table.concat (r, ",")
end

-- Return a list and hash of active jobs by kvs path
local function joblist_active (arg)
    local f = arg.flux
    local conf = arg.states
    local r, err = f:rpc ("job.list",
                          { max = arg.max,
                            include = to_string_list (conf and conf.include),
                            exclude = to_string_list (conf and conf.exclude)
                          })
    if not r then return nil, nil, err end
    local active = {}
    local results = {}
    for _,job in ipairs (r.jobs) do
        local path = job.kvs_path
        kvs_path_insert (job.jobid, path)
        active[path] = true
        table.insert (results, path)
    end
    return results, active
end

local function joblist_kvs (arg, exclude)
    local flux = require 'flux'
    local f = arg.flux
    if not f then f, err = flux.new () end
    if not f then return nil, err end

    local function visit (d, r)
        local results = r or {}
        if not d then return end
        local dirs = kvsdir_reverse_keys (d)
        for _,k in pairs (dirs) do
            local path = tostring (d) .. "." .. k
            local dir = f:kvsdir (path)
            if dir and (not exclude or not exclude[path]) then
                if dir.state then
                    -- This is a lwj dir, add to results table:
                    if include_state (arg.states, dir.state) then
                        table.insert (results, path)
                    end
                else
                    -- recurse to find lwj dirs lower in the directory tree
                    visit (dir, results)
                end
		if arg.max and #results >= arg.max then return results end
            end
        end
        return results
    end

    local dir, err = f:kvsdir ("lwj")
    if not dir then
        if err:match ("No such") then err = "No job information in KVS" end
        return nil, err
    end

    return reverse (visit (dir))
end

function wreck.joblist (arg)
    local results, active = {}, {}
    if not arg.kvs_only then
        results, active = joblist_active (arg)
        if results and arg.max then
            arg.max = arg.max - #results
        end
        if arg.active_only or arg.max == 0 then return results end
    end

    -- Append a list of jobs from the kvs to the active jobs
    local r, err = joblist_kvs (arg, active)
    if not r then return nil, err end

    return append (results or {}, r)
end

local function shortprog ()
    local prog = string.match (arg[0], "([^/]+)$")
    return prog:match ("flux%-(.+)$")
end

function wreck.new (arg)
    if not arg then arg = {} end
    local w = setmetatable ({ extra_options = {},
                              kvsdirs = {},
                              lwj_paths = {}}, wreck)
    w.prog = arg.prog or shortprog ()
    w.flux = arg.flux
    return w
end

return wreck

-- vi: ts=4 sw=4 expandtab
