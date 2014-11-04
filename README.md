_NOTE: The interfaces of flux-core are being actively developed
and are not yet stable._ The github issue tracker is the primary
way to communicate with the developers.

### flux-core

flux-core implements the communication layer, lowest level
services and interfaces for the Flux resource manager framework.
It consists of a distributed message broker and plug-in _comms modules_
that implement various distributed services.

#### Overview

A set of message broker instances are launched as a _comms session_.
Each instance has a rank numbered 0 to (size - 1).
Instances are interconnected with three overlay networks:
a k-ary tree rooted at rank 0 that is used for request/response
messages and data reductions, an event overlay that is used for
session-wide broadcasts, and a ring network that is used for debugging
requests.  Overlay networks are implemented using [ZeroMQ](http://zeromq.org).

The message broker natively supports the following services:
_logging_, which aggregates syslog-like log messages at rank 0;
_heartbeat_, which broadcasts a periodic event to synchronize
housekeeping tasks; _module loader_, which loads and unloads
comms modules; and _reparent_, which allows overlay networks to be
rewired on the fly.

flux-core also includes the following services implemented as 
comms modules: _kvs_, a distributed key-value store;  _live_,
a service that monitors overlay network health and can rewire around
failed broker instances; _modctl_, a distributed module control service;
_barrier_, a MPI-like barrier implementation; _api_, a routing service
for clients connecting to a broker instance via a UNIX domain socket;
and _wreck_ a remote execution service.

A number of utilities are provided for accessing these services,
accessible via the `flux` command front-end (see below),

Experimental programming abstractions are provided for various recurring
needs such as data reduction, multicast RPC, streaming I/O, and others.
A PMI implementation built on the Flux KVS facilitates scalable MPI launch.
A set of Lua bindings permits rapid development of Flux utilities and test
drivers.

flux-core is intended to be the first building block used in the
construction of a site-composed Flux resource manager.  Other building
blocks are being worked on and will appear in the
[flux-framework github organization](http://github.com/flux-framework)
as they get going.

Framework projects use the C4 development model pioneered in
the ZeroMQ project and forked as
[Flux RFC 1](http://github.com/flux-framework/rfc/blob/master/spec_1.adoc).
Flux licensing and collaboration plans are described in
[Flux RFC 2](http://github.com/flux-framework/rfc/blob/master/spec_2.adoc).
Protocols and API's used in Flux will be documented as Flux RFC's.

#### Building flux-core

flux-core requires the following packages to build:
```
zeromq4-devel >= 4.0.4   # built --with-libsodium
czmq-devel >= 2.2.0
munge-devel
json-c-devel
lua-5.1-devel
asciidoc
```
Spec files for building zeromq4 and czmq packages on a RHEL 6 based
system are provided for your convenience in foreign/rpm.

If you want to build the MPI-based test programs, make sure that
`mpicc` is in your PATH before you run configure.  These programs are
not built if configure does not find MPI.

```
./autogen.sh   # skip if building from a release tarball
./configure
make
make check
```
#### Bootstrapping a Flux comms session

A Flux comms session can be started for testing as follows.
First, generate a set of (personal) CURVE keys for session security:
```
cd src/cmd
./flux keygen
Saving $HOME/.flux/curve/client
Saving $HOME/.flux/curve/client_private
Saving $HOME/.flux/curve/server
Saving $HOME/.flux/curve/server_private
```
No administrator privilege is required to start a Flux comms
session as described below.

##### Single node session

To start a Flux comms session (size = 8) on the local node:
```
cd src/cmd
./flux start --size 8
```
A shell is spawned that has its environment set up so that Flux
commands can find the message broker socket.  When the shell exits,
the session exits.  Log output will be written to the file `cmbd.log`.

##### SLURM session

To start a Flux comms session (size = 64) on a cluster using SLURM,
first ensure that MUNGE is set up on your cluster, then:
```
cd src/cmd
./flux start -N 64
```
The srun --pty option is used to connect to the rank 0 shell.
When you exit this shell, the session terminates.
Log output will be written to the file `cmbd.log`.

#### Flux commands

To view the available Flux commands:
```
cd src/cmd
./flux -h
```
Most of these have UNIX manual pages as `flux-<sub-command>(1)`,
which can also be accessed using `./flux help <sub-command>`.
