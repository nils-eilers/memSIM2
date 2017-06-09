memSIM2
=======

This is a tool to load memory images onto the memSIM2 EPROM emulator
from momik.pl

Compilation
-----------

	autoreconf -i
	./configure
	make
    

Should build on Linux out of the box.
Other UN*X may needs some modifications. The are some Linux-specific hacks to make the serial port run at 460800 bps.


Installation
------------

	sudo make install


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

