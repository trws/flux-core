import re
import ctypes as ct
import flux_internal as fi

def gen_powers_of_two(x):
  i = 1
  for n in range(x + 1):
     yield i
     i <<= 1

class msgtype(object):
    REQUEST = 0x01
    RESPONSE = 0x02
    EVENT = 0x04
    KEEPALIVE = 0x08
    ANY = 0x0f
    MASK = 0x0f

class ArbitraryWrapperThunk(object):
    """ Wrap API functions to interpose the flux object where appropriate"""
    def __init__(self, handle, name):
        self.handle = handle
        self.fluxfun = False

        self.fun = None
        for lib in (fi.f,fi.f_lib):
            try:
                #try it bare
                self.fun = getattr(lib,name)
                break
            except AttributeError:
                pass
            try:
                #try it with the flux_ prepended
                self.fun = getattr(lib, "flux_" + name)
                self.fluxfun = True
                break
            except AttributeError:
                pass

        if self.fun is None:
            # Do it again to get a meaningful error
            fi.f.__getattribute__(name)

        def passthrough(self, *args):
            """pass updates through to the contained function,
            hope not to need this, but it will help set arguments if required"""
            return self.fun.__setattr__(*args)

        # late binding for convenience in __init__
        self.__setattr__ = passthrough

    def __call__(self, *args):
        if self.fluxfun:
            return self.fun(self.handle, *args)
        else:
            return self.fun(*args)

class Flux(object):
    handle = ct.POINTER(None)
    def __init__(self, handle):
        self.handle = handle

    #attempt to dispatch arbitrary un-defined interface functions:
    #   assumes that functions are of the form int [flux_]<name>(flux_t h[, args])
    def __getattr__(self, name):
        fun_container = ArbitraryWrapperThunk(self.handle, name)
        self.__dict__[name] = fun_container
        return fun_container

    def msghandler_add(self, message_type, topic, callback, arg):
        return fi.f.flux_msghandler_add(self.handle,
                                        message_type,
                                        topic,
                                        callback,
                                        arg)
    def reactor_start(self):
        return fi.f.flux_reactor_start(self.handle)

    def log(self, lev, fmt):
        return fi.f.flux_log(self.handle, lev, fmt)

    def err(self):
        fi.f.err()

    # def event_subscribe(self, topic):
    #     return fi.f.flux_event_subscribe(

#wrap message handler callbacks to hide flux_t conversion
def MsgHandler(fun):
        # return fi.MsgHandlerProto(fun)
        def callback_wrapper(handle_ptr, typemask, message_array, arg):
                #TODO: abstract out the message array, pyzmq should help
                return fun(Flux(handle_ptr), typemask, message_array, arg)

        typedfun = fi.MsgHandlerProto(callback_wrapper)
        return typedfun

