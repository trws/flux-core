###############################################################
# Copyright 2019 Lawrence Livermore National Security, LLC
# (c.f. AUTHORS, NOTICE.LLNS, COPYING)
#
# This file is part of the Flux resource manager framework.
# For details, see https://github.com/flux-framework.
#
# SPDX-License-Identifier: LGPL-3.0
###############################################################

import errno

from flux.util import check_future_error
from flux.wrapper import Wrapper, WrapperPimpl
from flux.core.inner import ffi, lib, raw
import _flux.new_binding as nb
from typing import Dict


# Reference count dictionary to keep futures with a pending `then` callback
# alive, even if there are no remaining references to the future in the user's
# program scope. When a callback is first set on the future, add an entry to the
# dictionary with the future itself as the key and a value of 1. Whenever a
# future's callback is run, decrement the count in the dictionary for that
# future, and whenever reset is called on a future, increment the counter. If
# the counter hits 0, delete the reference to the future from the dictionary.
_THEN_HANDLES: Dict["Future", int] = {}


@ffi.def_extern()
def continuation_callback(c_future, opaque_handle):
    try:
        py_future: "Future" = ffi.from_handle(opaque_handle)
        assert c_future == py_future.pimpl.handle
        py_future.then_cb(py_future, py_future.then_arg)
    finally:
        _THEN_HANDLES[py_future] -= 1
        if _THEN_HANDLES[py_future] <= 0:
            # allow future object to be garbage collected now that all
            # registered callbacks have completed
            py_future.cb_handle = None
            del _THEN_HANDLES[py_future]


class Future(WrapperPimpl):
    """
    A wrapper for interfaces that create and consume flux futures
    """

    class InnerWrapper(Wrapper):
        def __init__(
            self,
            handle=None,
            match=ffi.typeof("flux_future_t *"),
            filter_match=True,
            prefixes=None,
            destructor=raw.flux_future_destroy,
        ):
            # avoid using a static list as a default argument
            # pylint error 'dangerous-default-value'
            if prefixes is None:
                prefixes = ["flux_future_"]

            super(Future.InnerWrapper, self).__init__(
                ffi, lib, handle, match, filter_match, prefixes, destructor
            )

        def check_wrap(self, fun, name):
            func = super(Future.InnerWrapper, self).check_wrap(fun, name)
            return check_future_error(func)

    def __init__(self, future_handle, prefixes=None, pimpl_t=None):
        super(Future, self).__init__()
        if pimpl_t is None:
            pimpl_t = self.InnerWrapper
        self.pimpl = pimpl_t(handle=future_handle, prefixes=prefixes)
        self.then_cb = None
        self.then_arg = None
        self.cb_handle = None

    def error_string(self):
        try:
            errmsg = self.pimpl.error_string()
        except EnvironmentError:
            return None
        return errmsg.decode("utf-8") if errmsg else None

    def get_flux(self):
        # pylint: disable=cyclic-import
        import flux.core.handle

        flux_handle = nb.flux_future_get_flux(self)
        if flux_handle == ffi.NULL:
            return None
        handle = flux.core.handle.Flux(handle=flux_handle)
        # increment reference count to prevent destruction of the underlying handle
        # (which is owned by the future) when the flux handle is garbage collected
        handle.incref()
        return handle

    def then(self, callback, arg=None, timeout=-1.0):
        if self in _THEN_HANDLES:
            raise EnvironmentError(
                errno.EEXIST, "then callback already exists for this future"
            )
        if callback is None:
            raise ValueError("Callback cannot be None")

        self.then_cb = callback
        self.then_arg = arg
        self.cb_handle = ffi.new_handle(self)
        self.pimpl.then(timeout, lib.continuation_callback, self.cb_handle)

        # ensure that this future object is not garbage collected with a
        # callback outstanding. Particularly important for anonymous calls and
        # streaming RPCs.  For example, `f.rpc('topic').then(cb)`
        _THEN_HANDLES[self] = 1

        # return self to enable further chaining of the future.
        # For example `f.rpc('topic').then(cb).wait_for(-1)
        return self

    def reset(self):
        self.pimpl.reset()

        if self.cb_handle is not None:
            # ensure that this future object is not garbage collected with a
            # callback outstanding. Particularly important for streaming RPCs.
            _THEN_HANDLES[self] += 1

    get_reactor = nb.flux_future_get_reactor
    is_ready = nb.flux_future_is_ready
    wait_for_ = nb.flux_future_wait_for

    def wait_for(self, timeout: float = -1.0):
        self.wait_for_("bah")
