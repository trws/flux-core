import flux_internal
import flux

def mod_main_trampoline(name, encapsulated_handle, args):
    print "trampoline entered"
    #generate a flux wrapper class instance from the handle
    flux_instance = flux.Flux(flux_internal.get_handle_from_capsule(encapsulated_handle))
    print "flux instance retrieved"
    #import the user's module dynamically
    user_mod = __import__(name)
    print "user module loaded:", name
    #call into mod_main with a flux class instance and the argument dict
    #it might be more pythonic to unpack the args as keyword/positional arguments
    #to this function, but I think this is cleaner for now
    user_mod.mod_main(flux_instance, args)

