from __future__ import print_function

import re
import os
import sys
import math
import logging
import argparse
import json
import collections

try:
    collectionsAbc = collections.abc
except AttributeError:
    collectionsAbc = collections

from flux import util
import yaml


def create_resource(res_type, count, with_child=[]):
    assert isinstance(
        with_child, collectionsAbc.Sequence
    ), "child resource must be a sequence"
    assert not isinstance(with_child, str), "child resource must not be a string"
    assert count > 0, "resource count must be > 0"

    res = {"type": res_type, "count": count}

    if len(with_child) > 0:
        res["with"] = with_child
    return res


def create_slot(label, count, with_child):
    slot = create_resource("slot", count, with_child)
    slot["label"] = label
    return slot


def get_environment(l=[{}]):
    """
    Return current environment as specified by argument `l`, a list of
    dicts possibly provided by the --export command line option.
    If "ALL" is set always export all environment variables, else if
    "NONE" is set, export an empty environment.
    """
    environ = {}

    # Argument from --export is a list of dicts. Combine this list into
    #  a single "exports" dict:
    exports = {k: v for e in l for (k, v) in e.items()}

    # If --export option not used then return current environment:
    if not exports:
        return dict(os.environ)
    # If --export=NONE then return empty environment:
    elif "NONE" in exports:
        return {}
    # If --export=ALL,... then start with current environment, possibly
    # modified by export --export arguments:
    elif "ALL" in exports:
        del exports["ALL"]
        environ = dict(os.environ)

    # Set each env var to provided value, e.g. --export=FOO=bar,
    #  or current value if not provided, e.g. --export=FOO :
    for k, v in exports.items():
        try:
            environ[k] = v or os.environ[k]
        except KeyError:
            logger.error("Variable {} not found in current env".format(k))
            sys.exit(1)
    return environ


def create_slurm_style_jobspec(
    command,
    num_tasks,
    cores_per_task,
    num_nodes=0,
    walltime=None,
    environ=get_environment(),
):
    core = create_resource("core", cores_per_task)
    if num_nodes > 0:
        num_slots = int(math.ceil(num_tasks / float(num_nodes)))
        if num_tasks % num_nodes != 0:
            logging.warn(
                "Number of tasks is not an integer multiple of the number of nodes. "
                "More resources than required will be requested to satisfy your job."
            )
            task_count_dict = {"total": num_tasks}
        else:
            task_count_dict = {"per_slot": 1}
        slot = create_slot("task", num_slots, [core])
        resource_section = create_resource("node", num_nodes, [slot])
    else:
        task_count_dict = {"per_slot": 1}
        slot = create_slot("task", num_tasks, [core])
        resource_section = slot

    jobspec = {
        "version": 1,
        "resources": [resource_section],
        "tasks": [{"command": command, "slot": "task", "count": task_count_dict}],
        "attributes": {"system": {"cwd": os.getcwd(), "environment": environ}},
    }
    if walltime:
        jobspec["attributes"]["system"]["duration"] = walltime

    return jobspec


def validate_slurm_args(args):
    if args.nodes > args.ntasks:
        raise ValueError("Number of nodes greater than the number of tasks")

    if (
        args.time
        and re.match(r"^(\d+-)?\d+:\d+:\d+$", args.time) is None
        and re.match(r"^\d+(:\d+)?$", args.time) is None
    ):
        raise ValueError(
            "invalid time limit string format. "
            "Acceptable formats include minutes[:seconds], [days-]hours:minutes:seconds"
        )

    # TODO: is there any validation of the stdout redirection path that we can do?
    # IDEA: print a warning if the file already exists or if the parent dir doesn't exist


#  Convert slurm walltime string to floating point seconds
def slurm_walltime_to_duration(time):
    if not time:
        return None
    p1 = re.compile(
        r"^((?P<days>\d+)-)?"
        r"(?P<hours>\d+):"
        r"(?P<minutes>\d+):"
        r"(?P<seconds>\d+)$"
    )
    p2 = re.compile(r"^(?P<minutes>\d+)(:(?P<seconds>\d+))?$")

    t = 0.0
    m = p2.search(time) or p1.search(time)
    if m is None:
        return None
    vals = {k: float(v) for k, v in m.groupdict().items() if v is not None}
    if "days" in vals:
        t = t + vals["days"] * 60 * 60 * 24
    if "hours" in vals:
        t = t + vals["hours"] * 60 * 60
    if "minutes" in vals:
        t = t + vals["minutes"] * 60
    if "seconds" in vals:
        t = t + vals["seconds"]
    if t == 0.0:
        return None
    return t


def slurm_jobspec(args):
    if args.ntasks is None:
        args.ntasks = max(args.nodes, 1)

    try:
        validate_slurm_args(args)
    except ValueError as e:
        logger.error(str(e))
        sys.exit(1)
    t = slurm_walltime_to_duration(args.time)
    environ = get_environment(args.export)
    return create_slurm_style_jobspec(
        args.command, args.ntasks, args.cpus_per_task, args.nodes, t, environ
    )


def kv_list_split(s):
    """
    Split a key/value list with optional values, e.g. 'FOO,BAR=baz,...'
    and return a dict.
    """
    return dict((a, b) for a, _, b in [x.partition("=") for x in s.split(",")])


def get_slurm_common_parser():
    """
    Shared arguments amongst srun and sbatch.
    Used src/srun/libsrun/opt.c and src/sbatch/opt.c of the
    [SLURM repository](https://github.com/SchedMD/slurm.git) as reference
    """
    slurm_parser = argparse.ArgumentParser(add_help=False)
    slurm_parser.add_argument("-N", "--nodes", type=int, default=0)
    slurm_parser.add_argument("-n", "--ntasks", type=int)
    slurm_parser.add_argument("-c", "--cpus-per-task", type=int, default=1)
    slurm_parser.add_argument(
        "-t",
        "--time",
        help="time limit. Acceptable formats include minutes[:seconds], "
        "[days-]hours:minutes:seconds",
    )
    slurm_parser.add_argument("-o", "--output", help="location of stdout redirection")
    slurm_parser.add_argument(
        "--export",
        metavar="[ALL|NONE|VARS]",
        action="append",
        type=kv_list_split,
        default=[{}],
        help="List or specify environment variables to export",
    )
    slurm_parser.add_argument("command", nargs=argparse.REMAINDER)
    return slurm_parser


logger = logging.getLogger("flux-jobspec")


@util.CLIMain(logger)
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--format", choices=["json", "yaml"], default="json")

    subparsers = parser.add_subparsers()
    slurm_parser = get_slurm_common_parser()
    srun_parser = subparsers.add_parser(
        "srun", parents=[slurm_parser], help="subcommand for SLURM-style CLI arguments"
    )
    srun_parser.set_defaults(func=slurm_jobspec)

    args = parser.parse_args()

    if len(args.command) == 0:
        parser.error("command is required")
        sys.exit(1)

    jobspec = args.func(args)

    if args.format == "yaml":
        out = yaml.dump(jobspec)
    else:
        out = json.dumps(jobspec)
    print(out)


if __name__ == "__main__":
    main()
