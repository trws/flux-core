import re
import errno
import os

# A do-nothing class for checking for composition-based wrappers that offer a
# handle attribute
class WrapperBase(object):
  def __init__(self):
    self._handle = None

  @property
  def handle(self):
    return self._handle

  @handle.setter
  def handle(self, handle):
    self._handle = handle

class WrapperPimpl(WrapperBase):
  @property
  def handle(self):
    return self.pimpl.handle

  @handle.setter
  def handle(self, handle):
    self.pimpl.handle = handle

class Wrapper(WrapperBase):
  """
  Forms a wrapper around an interface that dynamically searches for undefined
  names, and can detect and pass a handle argument of specified type when it
  is found in the signature of an un-specified target function.
  """
  
  def __init__(self,
               ffi,
               lib,
               handle=None,
               match=None,
               filter_match=True,
               prefixes=[],
               wrap_none=True):
    super(Wrapper, self).__init__()

    self.ffi = ffi
    self.lib = lib
    self.handle = handle
    self.match = match
    self.filter_match = match
    self.prefixes = prefixes
    self.wrap_none = wrap_none

    self.NULL = ffi.NULL

  def callback(self, type_id):
    """ Pass-through to cffi callback mechanism for now"""
    return self.ffi.callback(type_id)

  def check_wrap(self, fun, name):
    try: #absorb the error if it is a basic type
      t = self.ffi.typeof(fun)
    except TypeError:
        return fun
    if self.match is not None:
      if t.kind == 'function' and t.args[0] == self.match: #first argument is of handle type
        handle_holder = fun
        def HandleWrapper(*args, **kwargs):
          return handle_holder(self.handle,*args,**kwargs)
        fun = HandleWrapper
      else:
        if self.filter_match:
          raise AttributeError("Flux Wrapper object masks function {} type: {} match: {}".format(name, t, self.match))
    if self.wrap_none:
      try: 
        # If this function takes at least one pointer
        if t.kind == 'function' and 'pointer' in [x.kind for x in t.args]:
          holder = fun
          def NoneWrapper(*args, **kwargs):
            # TODO: find a way to make this faster
            args = list(args)
            for i, v in enumerate(args):
              if v is None:
                args[i] = self.ffi.NULL
              elif isinstance(v, WrapperBase):
                # Unpack wrapper objects
                args[i] = v.handle
            for k, v in kwargs.items():
              if v is None:
                kwargs[k] = self.ffi.NULL
              elif isinstance(v, WrapperBase):
                # Unpack wrapper objects
                kwargs[k] = v.handle
            self.ffi.errno = 0
            result = holder(*args,**kwargs)
            # Convert errno errors into python exceptions
            err = self.ffi.errno
            if err != 0:
              raise EnvironmentError(err, os.strerror(err))
            return result if result != self.ffi.NULL else None
          fun = NoneWrapper
      except AttributeError:
          pass
    return fun

  def __getattr__(self, name):
    fun = None
    #try it bare
    try:
        fun = getattr(self.lib,name)
    except AttributeError:
        pass
    for prefix in self.prefixes:
      try:
          #wrap it
          fun = getattr(self.lib, prefix + name)
          break
      except AttributeError:
          pass
    if fun is None:
      # Do it again to get a good error
      getattr(self.lib, name)
    fun = self.check_wrap(fun, name)
    self.__dict__[name] = fun
    return fun
