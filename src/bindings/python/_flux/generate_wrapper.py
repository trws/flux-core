#!/usr/bin/env python3

from _flux._core import lib, ffi
import sys

PREFIX = "flux_"
HANDLE_TYPES = {
    "struct flux_handle_struct *": "'Flux'",
    "struct flux_watcher *": "'Watcher'",
    "struct flux_future *": "'Future'",
    "struct flux_msg *": "'Message'",
}

s = set()

PRELUDE = """
from ._core import lib, ffi

import errno
import os
import typing

if typing.TYPE_CHECKING:
    from flux.core.handle import Flux
    from flux.core.watchers import Watcher
    from flux.future import Future
    from flux.message import Message
from flux.wrapper import WrapperBase


Stringish = typing.Optional[typing.Union[str,bytes,ffi.CData]]
"""
CONSTANT = """
{name} = {exp}
"""

OUT = [PRELUDE]


def is_ptr(t):
    return t.kind in ("pointer", "array")


def is_void(t):
    return t == ffi.typeof("void")


def is_str(t):
    return t in (ffi.typeof("char *"), ffi.typeof("const char *"))


def is_integral(t):
    return t in [
        ffi.typeof(x)
        for x in (
            "_Bool",
            "char",
            "int",
            "short",
            "long",
            "long long",
            "uint32_t",
            "int32_t",
            "int64_t",
            "uint64_t",
        )
    ]


def is_float(t):
    return t in [ffi.typeof(x) for x in ("float", "double")]


for item in lib.__dict__:
    if isinstance(getattr(lib,item), int):
        OUT.append(CONSTANT.format(name=item, exp=f"lib.{item}"))

    if item.startswith(PREFIX):
        note = []
        f = getattr(lib, item)
        try:
            t = ffi.typeof(f)
        except TypeError:
            continue

        args = []
        arg_names = []
        arg_trans = []
        for i, a in enumerate(t.args):
            arg_names.append(f"a{i}")
            if a.kind == "pointer":
                arg_trans.append(
                    f"""
    if a{i} is None:
        a{i} = ffi.NULL
        """
                )

            arg_type = "ffi.CData"
            if is_integral(a):
                arg_type = "int"
            elif is_float(a):
                arg_type = "float"
            elif is_str(a):
                arg_type = "Stringish"
                arg_trans.append(
                    f"""
    if isinstance(a{i}, str):
        a{i} = a{i}.encode("utf-8")
"""
                )
            elif a in map(ffi.typeof, HANDLE_TYPES):
                arg_type = f"typing.Union[{HANDLE_TYPES[a.cname]},ffi.CData]"

            if a.kind == "pointer":
                arg_trans.append(
                    f"""
    if isinstance(a{i}, WrapperBase):
        a{i} = a{i}.handle
"""
                )

            note.append(f"    A{i} C type: {a.cname}")
            args.append(f"a{i}: {arg_type}")

        res_check = ""
        if t.result.kind == "pointer":
            res = (
                "typing.Optional[ffi.CData]"
            )  # super generic, be more specific as possible
        else:
            res = "ffi.CData"  # super generic, be more specific as possible
        if is_void(t.result):
            res = "None"
        elif is_integral(t.result):
            res = "int"
            res_check = """
    if res < 0:
        errnum = ffi.errno
        if errnum != 0:
            raise EnvironmentError(
                errnum,
                "{}: {}".format(errno.errorcode[errnum], os.strerror(errnum)),
            )
"""
        elif is_str(t.result):
            res = "typing.Optional[bytes]"
            res_check = """
    if res == ffi.NULL:
        res = None
"""
        elif is_float(t.result):
            res = "float"

        note.append(f"    Return C type: {str(t.result)}")

        nl = "\n"
        OUT.append(
            f"""
def {item}({", ".join(args)}) -> {res}:
    '''
{nl.join(note)}
    '''
    # Translate arguments if necessary
    {nl.join(arg_trans)}

    res = lib.{item}({", ".join(arg_names)})

    {res_check}

    return res
        """
        )

        # try:
        #     print(item, getattr(lib, item), ffi.typeof(getattr(lib,item)))
        # except TypeError:
        #     print("FFI ERROR on:", item)

print("\n".join(OUT))
