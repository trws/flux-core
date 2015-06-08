import ctypes as ct
# Try loading the external so
f_lib = ct.CDLL("/g/g12/scogland/projects/flux/flux-core/src/common/.libs/libflux-core.so", mode=ct.RTLD_GLOBAL)
# Load the currently executing binary as a module
f = ct.CDLL(None)
f.flux_log.restype = ct.c_int
#no direct varargs support, only take a string, use format externally
f.flux_log.argtypes = [ct.c_void_p, ct.c_long, ct.c_char_p]

f.flux_event_subscribe.restype = ct.c_int
f.flux_event_subscribe.argtypes = [ct.c_void_p, ct.c_char_p]
f.flux_event_subscribe.restype = ct.c_int
f.flux_event_subscribe.argtypes = [ct.c_void_p, ct.c_char_p]
f.flux_reactor_start.restype = ct.c_int
f.flux_reactor_start.argtypes = [ct.c_void_p,]

MsgHandlerProto = ct.CFUNCTYPE(ct.c_int, ct.c_void_p, ct.c_int, ct.POINTER(ct.c_void_p), ct.c_void_p)

f.flux_msghandler_add.restype = ct.c_int
f.flux_msghandler_add.argtypes = [ct.c_void_p, ct.c_int, ct.c_char_p, MsgHandlerProto, ct.c_void_p]

f.json_object_to_json_string.restype = ct.c_char_p
f.json_object_to_json_string.argtypes = [ct.c_void_p]


#this should *not* be necessary
#may be a side-effect of LOCAL_LOAD of module?
#TODO: add as a configuration option or find a way to fix symbol lookup
f_py = ct.PyDLL("libpython2.7.so.1.0")
f_py.PyCapsule_GetPointer.argtypes = [ct.py_object, ct.c_char_p]
f_py.PyCapsule_GetPointer.restype = ct.c_void_p
f_py.PyCapsule_New.argtypes = [ct.c_void_p, ct.c_char_p, ct.c_void_p]
f_py.PyCapsule_New.restype = ct.py_object

def get_handle_from_capsule(capsule):
    return f_py.PyCapsule_GetPointer(capsule, "flux-handle")

