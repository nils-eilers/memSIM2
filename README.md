memSIM2
=======

This is a tool to load memory images onto the memSIM2 EPROM emulator
from momik.pl

Compilation
-----------

	autoreconf -i
	./configure
	make

The install location defaults to `/usr/local/bin`. Type `./configure --help` for a variety of options if you want
to change this.

Should build on Linux out of the box.
Other UN*X may needs some modifications. The are some Linux-specific hacks to make the serial port run at 460800 bps.


Installation
------------


### Installing the binary

	sudo make install
    
installs the binary to the path you defined by the configuration script, usually
`/usr/local/bin`.


### Gain access rights to the simulator

If you try to access the simulator and get an error message `permission denied`, you
probably don't have the required rights to access the EPROM simulator.
You may gain these rights for example by adding your user to the appropiate group.

Ensure, the simulator is *not* plugged in. Plug it in then, then issue

	sudo dmesg
    
You should get a number of messages, the last ones should tell you the device name used by
the EPROM simulator, `/dev/ttyUSB0` in this case:

```
[ 4789.932478] usbserial: USB Serial support registered for FTDI USB Serial Device
[ 4789.932504] ftdi_sio 1-1:1.0: FTDI USB Serial Device converter detected
[ 4789.932520] usb 1-1: Detected FT-X
[ 4789.932659] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB0
```

Show the group of that device:

	ls -l /dev/ttyUSB0

which gives here:

	crw-rw---- 1 root uucp 188, 0  9. Jun 07:20 /dev/ttyUSB0

so your user must be in the `uucp` group to gain the required access rights.
Run `groups` to show the current set of groups you're part of. If your user name
is not a member of the group used by the EPROM simulator, you may add this group
to your user like so:

	sudo usermod -aG uucp <your-user-name>

Run `su <your-user-name>` to apply the new group immediately. As an
alternative, you may log out and in again.


Usage
-----

	memSIM2 -h

gives this help text:

```
Usage: [OPTION].. FILE
Upload image file to memSIM2 EPROM emulator

Options:
	-d DEVICE     Serial device
	-m MEMTYPE    Memory type (2764,27128,27256,27512,27010,27020,27040)
	-r RESETTIME  Time of reset pulse in milliseconds.
	              > 0 for positive pulse, < 0 for negative pulse
	-e            Enable emulation
	-h            This help
```

Copyright
---------

Written by Simon Berg <ksb@fluffware.se>  
memSIM2 is free software, licensed under GPL v2 or later

