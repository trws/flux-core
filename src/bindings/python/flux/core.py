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

raw = Core()

class Watcher(object):
  def __init__(self):
    pass

  def __enter__(self):
    """Allow this to be used as a context manager"""
    self.start()
    return self

  def __exit__(self, type_arg, value, tb):
    """Allow this to be used as a context manager"""
    self.stop()
    return False

  def __del__(self):
    if self.handle is not None:
      self.destroy()

@ffi.callback('flux_msg_watcher_f')
def MsgHandlerWrapper(handle_trash, m_watcher_t, msg_handle, opaque_handle):
  watcher = ffi.from_handle(opaque_handle)
  ret = watcher.cb(watcher.fh, watcher, Message(handle=msg_handle, destruct=True), watcher.args)

class MessageWatcher(Watcher):
  def __init__(self,
      flux_handle,
      type_mask,
      callback,
      topic_glob='*',
      match_tag=flux.FLUX_MATCHTAG_NONE,
      bsize=0,
      args=None):
    self.handle = None
    self.fh = flux_handle
    self.cb = callback
    self.args = args
    wargs = ffi.new_handle(self)
    c_topic_glob = ffi.new('char[]', topic_glob)
    match = ffi.new('struct flux_match *', {
      'typemask' : type_mask,
      'matchtag' : match_tag,
      'bsize' : bsize,
      'topic_glob' : c_topic_glob,
      })
    self.handle = raw.flux_msg_watcher_create(match[0], MsgHandlerWrapper, wargs)

  def start(self):
    raw.flux_msg_watcher_start(self.fh.handle, self.handle)

  def stop(self):
    raw.flux_msg_watcher_stop(self.fh.handle, self.handle)

  def destroy(self):
    if self.handle is not None:
      raw.flux_msg_watcher_destroy(self.handle)
      self.handle = None

@ffi.callback('flux_timer_watcher_f')
def TimeoutHandlerWrapper(handle_trash, timer_watcher_s, revents, opaque_handle):
  watcher = ffi.from_handle(opaque_handle)
  ret = watcher.cb(watcher.fh, watcher, revents, watcher.args)

class TimerWatcher(Watcher):
  def __init__(self,
      flux_handle,
      after,
      callback,
      repeat=0,
      args=None,
      ):
    self.fh = flux_handle
    self.after = after
    self.repeat = repeat
    self.cb = callback
    self.args = args
    self.handle = None
    wargs = ffi.new_handle(self)
    self.handle = raw.flux_timer_watcher_create(float(after), float(repeat), TimeoutHandlerWrapper, wargs)

  def start(self):
    raw.flux_timer_watcher_start(self.fh.handle, self.handle)

  def stop(self):
    raw.flux_timer_watcher_stop(self.fh.handle, self.handle)

  def destroy(self):
    if self.handle is not None:
      raw.flux_timer_watcher_destroy(self.handle)
      self.handle = None


class Flux(Wrapper):
  """
  The general Flux handle class, create one of these to connect to the
  nearest enclosing flux instance
  >>> flux.Flux() #doctest: +ELLIPSIS
  <flux.core.Flux object at 0x...>
  """
  def __init__(self, url=ffi.NULL, handle=None):
    self.external = False
    self.handle = None
    if handle is None:
      handle = raw.flux_open(url, 0)
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
    if not self.external and self.handle is not None:
      raw.flux_close(self.handle)

  def log(self, level, fstring):
    """Log to the flux logging facility
       
       :param level: A syslog log-level, check the syslog module for possible values
       :param fstring: A string to log, C-style formatting is *not* supported
    """
    # Short-circuited because variadics can't be wrapped cleanly
    lib.flux_log(self.handle, level, fstring)

  def send(self, message, flags=0):
    """Send a pre-constructed flux message"""
    if isinstance(message, Message):
      message = message.handle
    return self.flux_send(message, flags)

  def recv(self, type_mask=flux.FLUX_MSGTYPE_ANY, match_tag=flux.FLUX_MATCHTAG_NONE, topic_glob=None, bsize=0, flags=0):
    """ Receive a message, returns a flux.Message containing the result or None"""
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
    """ Create and send an RPC in one step """
    r = RPC(self, topic, payload, nodeid, flags)
    return r.get()

  def rpc_create(self, topic, payload=None, nodeid=flux.FLUX_NODEID_ANY, flags=0):
    """ Create a new RPC object """
    return RPC(topic, payload, nodeid, flags)

  def event_create(self, topic, payload=None):
    """ Create a new event message.
        
        :param topic: A string, the event's topic
        :param payload: If a string, the payload is used unmodified, if it is another type json.dumps() is used to stringify it
    """
    return Message.from_event_encode(topic, payload)

  def event_send(self, topic, payload=None):
    """ Create and send a new event in one step """
    return self.send(self.event_create(topic, payload))

  def event_recv(self, topic=None, payload=None):
    return self.recv(type_mask=lib.FLUX_MSGTYPE_EVENT, topic_glob=topic)

  def msg_watcher_create(self,
      callback,
      type_mask=lib.FLUX_MSGTYPE_ANY,
      pattern='*',
      args=None,
      match_tag=flux.FLUX_MATCHTAG_NONE,
      bsize=0):
    return MessageWatcher(self, type_mask, callback, pattern, match_tag, bsize, args)

  def timer_watcher_create(self, after, callback, repeat=0.0, args=None):
    return TimerWatcher(self, after, callback, repeat=repeat, args=args)





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
    user_mod = None
    try:
      user_mod = importlib.import_module('flux.modules.' + name, 'flux.modules')
    except ImportError: #check user paths for the module
      user_mod = importlib.import_module(name)

    # print "user module loaded:", name
    #call into mod_main with a flux class instance and the argument dict
    #it might be more pythonic to unpack the args as keyword/positional arguments
    #to this function, but I think this is cleaner for now
    user_mod.mod_main(flux_instance, **args)

#replace the module with an instance of the Flux class
# ref = sys.modules[__name__]
# sys.modules[__name__] = Flux()
