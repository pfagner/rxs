# RXS (Remote eXchange System)
The client-server system RXS allows users to work with file handling functions 
located on remote file system as if they were on a local file system.
The functioning of the RXS system is based on the TCP/IP architecture of the 
client-server model.

### Disclaimer
The current status is not a reflection of the completeness or stability of the code, 
it indicates that the project is not yet fully completed.

## Motivation
- The RXS protocol provides access to the remote file system via the network using TCP/IP.
- The client generates and sends requests to the server through the TCP/IP stack.
- The server receives a request from the client, which is passed to the 
local access routine, which provides access to file handling functions on the local drive.
- Processing of each request takes some time, depending on the quality of the communication 
channel and the load on the server, the length of time can vary both comparable to the performance 
of the local disk, and in the direction of increasing time.
- The server and the client filter the input stream, allocating packets along the packet boundary 
and filtering out invalid data. The server listens and works with client connections on the same 
address and port, respectively, the entire stream from clients (connection requests, commands and data) 
is sent to the same address and port of the server.
- The intensity of the data stream coming from the client to the server can be set on the client 
as a constant value by setting the value of the data portion and the time interval between 
sending each portion, thereby reducing the load of the incoming data stream on the server 
and the load to the data channel.
- It is possible to implement adaptive regulation by setting the transfer 
values dynamically, transferring the required data transfer parameters from the server to the client, 
depending on the load of the input stream on the server.

## Supported Architectures
x86, x86-64, ARM

## Operating Systems
Linux, QNX

## Dependencies
* Cmake >= 2.8

## Folder structure
```
├─config/ - user configuration
├─include/ - include files
├─lib/ - lib source files
├─scripts/
│  ├─srv/ - scripts for build the server
│  └─cli/ - scripts to build the client
└─tools/ - source files
```
## Build and install
### Build and install RXS server with a static library
```
$ mkdir build && cd $_
$ cp <path_to_src>/scripts/srv/build_install.sh .
$ sh ./build_install.sh <path_to_src>
```
In case of successful installation, the following files will be installed (see "Installed files")
### Building an RXS server with a static library
```
$ mkdir build && cd $_
$ cp <path_to_src>/scripts/srv/build_release.sh .
$ sudo sh ./build_release.sh <path_to_src>
```
In case of successful built, the folder 'rxs_release_YYYYMMDD_HHMMSS' will be contains:
```
  - installation script: install.sh
  - archive file: rxs.tar
```
### Installing an RXS server with a static library
```
$ cd 'rxs_release_YYYYMMDD_HHMMSS'
$ sh ./install.sh ./rxs.tar
```
In case of successful installation, the following will be installed:
```
├─/usr/
│  └─sbin/
│     ├─rxsd -- server
│     └─rxsc -- client
├─/etc/
│   ├─rxs/
│   │  └─rxs_users -- users list
│   └─rc.d/
│      └─init.d/
│         └─rxsd.sh -- startup script
```
### Build and Install RXS Client with static library
```
$ mkdir build && cd $_
$ cp <path_to_src>/scripts/cli/build_install.sh .
$ sh ./build_install.sh <path_to_src>
```
In case of successful installation, the following files will be installed: 
```
- executable file:
  /usr/sbin/rxsc
- configuration files
- startup scripts
```
### Build and install RXS server as shared libraries
```
$ mkdir build && cd $_
$ cp script/build_libs.sh .
$ sudo sh ./build_libs.sh <path_to_src>
```
In case of successful assembly, the following files will be installed:
```
/usr/local/lib/rxs/librxs_protocol.so
/usr/local/lib/rxs/librxs_structure.so
```

### Build RXS server/client with shared library
1) Build a project with the flag "BUILD_SHARED_LIBS":
```
$ cmake -DBUILD_SHARED_LIBS=ON <path_to_src>
$ make
```
2) After executing the command, for example, in the assembly folder "rxs_build", following files will be created:
```
  build/lib/protocol/librxs_protocol.so -- shared library
  build/lib/structure/librxs_structure.so -- shared library
  build/tools/server/rxs_srv -- server
  build/tools/client/rxs_cli -- client
```
3) On the target computer, place the shared libraries "librxs_protocol.so", "librxs_structure.so" in the appropriate folder, for example, "/usr/local/lib/rxs":
4) Add the library and update the dynamic linking
```
$ nano /etc/ld.so.conf
  /usr/local/lib/rxs
$ sudo ldconfig
```
5) Put the server and client file in your folders and run them

## Start/Stop RXS client/server
### Start server
```
$/etc/rc.d/init.d/rxsd.sh start --mode=daemon --addr_rxs=192.168.0.1 --port_rxs=1301 --addr_allowed=193.28.21.5 --file_users=/etc/rxs/rxs_users
```
### Stop server
```
$/etc/rc.d/init.d/rxsd.sh stop
```
### Start client
Put file to other side:
```
$/usr/sbin/rxsc put username:password@address:port ./local_file ./remote_file
```
Get file from other side:
```
$/usr/sbin/rxsc get username:password@address:port ./local_file ./remote_file
```
Control to remote terminal:
```
$/usr/sbin/rxsc cli username:password@address:port
```
## License and Copyright
This code is copyright (c) 2012 - 2023 v.arteev
Licensed under the MIT, please see LICENSE file for more information.
