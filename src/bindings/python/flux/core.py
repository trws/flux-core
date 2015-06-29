import importlib
import sys
import json
from flux.wrapper import Wrapper, WrapperPimpl
from flux._core import ffi, lib
import flux
from flux.rpc import RPC
from flux.message import Message



class Core(Wrapper):
  """ 
  Generic Core wrapper, you probably do not want or need one of these.
  """

  def __init__(self):
    """Set up the wrapper interface for functions prefixed with flux_"""
    super(Core, self).__init__(ffi,
        lib,
        prefixes=[
          'flux_',
          'FLUX_',
          ])

@ffi.callback('FluxMsgHandler')
def MsgHandlerWrapper(handle_trash, type_mask_cb, msg_handle, opaque_handle):
  (cb, handle, args) = ffi.from_handle(opaque_handle)
  ret = cb(handle, type_mask_cb, Message(handle=msg_handle[0], destruct=True), args)
  return ret if ret is not None else 0

@ffi.callback('FluxTmoutHandler')
def TimeoutHandlerWrapper(handle_trash, opaque_handle):
  (cb, handle, args) = ffi.from_handle(opaque_handle)
  ret = cb(handle, args)
  return ret if ret is not None else 0

raw = Core()
class Flux(Wrapper):
  def __init__(self, handle=None):
    self.msghandlers = {}
    self.timeouts = {}
    if handle is None:
      self.external = False
      handle = lib.flux_open(ffi.NULL, 0)
    else:
      self.external = True
    super(self.__class__, self).__init__(ffi, lib, handle=handle,
                                     match=ffi.typeof(lib.flux_open).result,
                                     prefixes=[
                                       'flux_',
                                       'FLUX_',
                                       ],
                                     )

  def __del__(self):
    if not self.external:
      self.close()

  def log(self, level, fstring):
    """Log to the flux logging facility"""
    # Short-circuited because variadics can't be wrapped cleanly
    lib.flux_log(self.handle, level, fstring)

  def send(self, message, flags=0):
    if isinstance(message, Message):
      message = message.handle
    return self.flux_send(message, flags)

  def recv(self, type_mask=flux.FLUX_MSGTYPE_ANY, match_tag=flux.FLUX_MATCHTAG_NONE, topic_glob=None, bsize=0, flags=0):
    """ Receive a message, returns a Message containing the result or None"""
    match = ffi.new('struct flux_match *',{
      'typemask' : type_mask,
      'matchtag' : match_tag,
      'topic_glob' : topic_glob if topic_glob is not None else ffi.NULL,
      'bsize' : bsize,
      })
    handle = self.flux_recv(match[0], flags)
    if handle is not None:
      return Message(handle=handle)
    else:
      return None

  def rpc_send(self, topic, payload=ffi.NULL, nodeid=flux.FLUX_NODEID_ANY, flags=0):
    r = RPC(self, topic, payload, nodeid, flags)
    return r.get()

  def rpc_create(self, topic, payload=None, nodeid=flux.FLUX_NODEID_ANY, flags=0):
    return RPC(topic, payload, nodeid, flags)

  def event_create(self, topic, payload=None):
    return Message.from_event_encode(topic, payload)

  def event_send(self, topic, payload=None):
    return self.send(self.event_create(topic, payload))

  def event_recv(self, topic=None, payload=None):
    return self.recv(type_mask=lib.FLUX_MSGTYPE_EVENT, topic_glob=topic)

  def msghandler_add(self, callback, type_mask=lib.FLUX_MSGTYPE_ANY, pattern='*', args=None):
    packed_args = (callback, self, args)
    # Save the callback arguments to keep them from getting collected
    self.msghandlers[(type_mask, pattern)] = packed_args
    arg_handle = ffi.new_handle(packed_args)
    return self.flux_msghandler_add(type_mask, pattern, MsgHandlerWrapper, arg_handle)

  def msghandler_remove(self, type_mask=lib.FLUX_MSGTYPE_ANY, pattern='*'):
    self.flux_msghandler_remove(type_mask, pattern)
    self.msghandlers.pop((type_mask, pattern), None)

  def timeout_handler_add(self, milliseconds, callback, oneshot=True, args=None):
    packed_args = (callback, self, args)
    arg_handle = ffi.new_handle(packed_args)
    timeout_id = self.flux_tmouthandler_add(milliseconds, oneshot, TimeoutHandlerWrapper, arg_handle)
    # Save the callback arguments to keep them from getting collected
    self.timeouts[timeout_id] = packed_args
    return timeout_id

  def timeout_handler_remove(self, timer_id):
    self.flux_tmouthandler_remove(timer_id)
    # Remove handle to stored arguments
    self.timeouts.pop(timer_id, None)




class Sec(Wrapper):
  def __init__(self, handle=None):
    if handle is None:
      self.external = False
      handle = lib.flux_sec_create(type_id)
    else:
      self.external = True
    super(self.__class__, self).__init__(ffi, lib, 
                                     handle=handle,
                                     match=ffi.typeof(lib.flux_sec_create).result,
                                     prefixes=['flux_sec_'],
                                     )
  
def mod_main_trampoline(name, int_handle, args):
    # print "trampoline entered"
    #generate a flux wrapper class instance from the handle
    flux_instance = Flux(handle = lib.unpack_long(int_handle))
    # print "flux instance retrieved, loading:", name
    #impo__import__('flux.modules.' + name)rt the user's module dynamically
    user_mod = importlib.import_module('flux.modules.' + name, 'flux.modules')

    # print "user module loaded:", name
    #call into mod_main with a flux class instance and the argument dict
    #it might be more pythonic to unpack the args as keyword/positional arguments
    #to this function, but I think this is cleaner for now
    user_mod.mod_main(flux_instance, **args)

#replace the module with an instance of the Flux class
# ref = sys.modules[__name__]
# sys.modules[__name__] = Flux()
