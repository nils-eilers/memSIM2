# memSIM2 for Linux
This is a software tool to load memory images onto the memSIM2 EPROM emulator from http://momik.pl .

MemSIM2 is an EPROM emulator. It emulates EPROMs from 2764 (8K/28 pin) to 27040 (4M/32 pin). This is accomplished by downloading your code with this tool.

**WARNING:** Do not use the memSIM2 in an EPROM burner. It will be destroyed by the programming voltages. To 'program' the EPROM just use the command line to update the emulators contents via the USB. Also memSIM2 may work fine in place of EEPROM  but it can't be programmed in system like an EEPROM or Flash chip can.

**WARNING:** Take great care when using an adapter to simulate 2716 (2K) or 2732 (4K) ERPOMs (24 pin) some of the really odd chips put the voltages in different places or, worse yet, use voltages (+5, -5 and +12v) that will destroy the memSIM2. See the TMS2716 below as an example of an EPROM with triple voltages.

# Build
```
autoreconf -i
./configure
make
make install

memSIM -h
```

Should build on Linux and most POSIX systems out of the box.

Installation
------------


### Installing the binary
```
sudo make install
```

This installs the binary to the path you defined by the configuration script, usually '/usr/local/bin'.


### Gain access rights to the simulator

If you try to access the simulator and get an error message 'permission denied', you probably don't have the required rights to access the EPROM simulator. You may gain these rights for example by adding your user to the appropriate group.

Ensure, the simulator is *not* plugged in. Plug it in then, then issue
```
	sudo dmesg
```
You should get a number of messages, the last ones should tell you the device name used by
the EPROM simulator, '/dev/ttyUSB0' in this case:

```
[ 4789.932478] usbserial: USB Serial support registered for FTDI USB Serial Device
[ 4789.932504] ftdi_sio 1-1:1.0: FTDI USB Serial Device converter detected
[ 4789.932520] usb 1-1: Detected FT-X
[ 4789.932659] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB0
```

Show the group of that device:
```
	ls -l /dev/ttyUSB0
```
which gives here:
```
	crw-rw---- 1 root uucp 188, 0  9. Jun 07:20 /dev/ttyUSB0
```
so your user must be in the 'uucp' group to gain the required access rights.
Run 'groups' to show the current set of groups you're part of. If your user name
is not a member of the group used by the EPROM simulator, you may add this group
to your user like so:
```
	sudo usermod -aG uucp <your-user-name>
```
Run 'su <your-user-name>' to apply the new group immediately. As an
alternative, you may log out and in again.


Ensure, the simulator is *not* plugged in. Plug it in then, then issue
```
	sudo dmesg
```
You should get a number of messages, the last ones should tell you the device name used by
the EPROM simulator, '/dev/ttyACM0' in this case:

```
[ 4789.932478] usbserial: USB Serial support registered for FTDI USB Serial Device
[ 4789.932504] ftdi_sio 1-1:1.0: FTDI USB Serial Device converter detected
[ 4789.932520] usb 1-1: Detected FT-X
[ 4789.932659] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB0
```

Show the group of that device:
```
	ls -l /dev/ttyACM0
```
which gives here:
```
	crw-rw---- 1 root dialout 166,  0 Mar 19 07:14 /dev/ttyACM0
```
so your user must be in the 'dialout' group to gain the required access rights.
Run 'groups' to show the current set of groups you're part of. If your user name
is not a member of the group used by the EPROM simulator, you may add this group
to your user like so:
```
	sudo usermod -aG dialout <your-user-name>
```
Run 'su <your-user-name>' to apply the new group immediately. As an
alternative, you may log out and in again.


# Notes
My notes and ramblings. The code is a work in progress.

The memSIM2 can emulate up to 512Kbyte EPROMs (32 Pin), the smallest is an 8K 2764 (28 Pin)

The memSIM2 comes with a 32 pin cable also says that a 28 pin cable is available.

I've stacked the 32 pin end of the cable to a high quality machine socket to the socket. I learned that trick from using CPU emulators. It gives a sturdier set of pins to insert into the EPROM sockets.

I've got the 32 pin cable version of the memSIM2 and I also need to work with 28 pin EPROMS. I see in the flyer that it supports a Vcc of 2.7v to 5.0v. It is possible to connect the 32 pin cable to the 28 pin socket by using an additional 28 pin machine socket using the lower 28 pins of the 32 pin cable (pins 1, 2, 31 & 32, the top 4 pins are exposed). (Yes)

Similarly it is possible to also connect the 32 or 28 pin cable to a 24 pin socket by again using the lower 24 pins (pins 1, 2, 3, 4, 29, 30, 31 & 32, the top 8 pins are exposed). Be careful not to short the exposed pins to anything else. Also I'm pretty sure that the Vcc pin 24 needs to be connected to pin 28 of the cable for voltage sensing. (Untested)

- When loading code into the simulated EPROM where does it get loaded?
- I can have multiple address sections that may not be contigous. Will it load correctly?
- How does it know where the EPROM starts?
- I'm not sure I have this coded with the s-record.
- Need to provide pictures of the stacked machine sockets adapters.

# Help

MemSIM2 supports the Intel hex format and straight binary. Motorola S19 format is a work in progress.

```
$ memSIM2 -h
Usage: [OPTION].. FILE
Upload image file to memSIM2 EPROM emulator.  Where file can be .ihx or .bin.

Options:
        -d DEVICE     Serial device
        -m MEMTYPE    Memory type (2764 -8K, 27128 - 16K, 27256 - 32K, 27512 - 64K,
                      27010 - 128K, 27020 - 256K, 27040 - 512K)
                      2764-27512 are 28 Pin, 27010-27040 are 32 pin.
                      defaults to 27512 (?)
        -r RESETTIME  Time of reset pulse in milliseconds.
                      > 0 for positive pulse, < 0 for negative pulse
        -e            Enable emulation
        -o BYTES      Specify an offset value with different meaning for:
                      binary files: skip first n bytes of file
                      Hex files: start address in memory map of simulated memory chip
        -h            This help
$
```

## Usage
-----

If the provided defaults work for you, the usage is as easy as
plugging everything in while your devices are powered off, switching
them on and running
```
        memsim2 imagefile.ext
```
to upload and reset the device. The kind of imagefile is solely
detected by its file extension. These formats are currently supported:

| Extension                 | Image type        |
|---------------------------|-------------------|
| .bin                      | Raw binary files  |
| .hex                      | Intel Hex files   |
| .s19 .s28 .s37 .srec .mot | Motorola S-Record |


## Raw binary files
----------------

There's not too much to say about these. Usually they're a binary dump
of a memory chip. The type of memory chip (e.g. 2764) will be auto-detected
if the data size matches the size of a supported chip. If not, a warning
will be written to the screen and the next larger chip will be simulated.

It can't be easier than just simulating one chip of the very same size
of the provided binary data, for example:
```
        memsim2 myrom.bin
```
If you don't provide a specific chip to simulate, it will be auto-detected
by the amount of binary data but you may of course specify one:
```
        memsim2 -m 27128 os16k.bin
```
If your build system emits a larger binary than you actually would like
to simulate, you may specify an offset to the beginning of the file
indicating where the simulated data starts and the type of the memory
chip to simulate. As an example, let us assume, your operating system
occupies 16 KB and is spread across two 2764 chips of 8 KB each. You're
currently working on code in the upper half, so you'd like to simulate
the upper memory while your build system provides the full 16 KB:
```
        memsim2 -m 2764 -o 8192 os16k.bin
```
If you prefer, you may specify the offset in hex if you prefix the
value with '0x' or '0X' such as used in the C programming language:
```
        memsim2 -m 2764 -o 0x2000 os16k.bin
```
This example will simulate a 2764 chip (8 KB size). The -o option specifies
an offset of 8192 to make the simulator skip the first 8192 bytes of the
provided file os16k.bin, so that only the upper half of the binary file is
transferred to the simulated memory.

For historical reasons, it's even possible to provide the value in
octal but I won't go into details here. You only need to know that
a leading 0 triggers octal interpretation so if you mean 10 decimal
you need to write it as 10 and not as 0010 because that would be
octal 0010 = decimal 8 which would obviously break whatever you
intend to do.


## Intel hex
---------

If the file extension is .hex, the file will be interpreted as
Intel hex file:
```
        memsim2 mydata.hex
```
These may come in three variants:

- I8HEX (16 bit addresses)
- I16HEX (20 bit addresses)
- I32HEX (32 bit addresses)

Intel hex files usually consist of several records (text lines) with
data, each secured with a checksum and provided with an start address
indicating where the following data bytes should get stored.

Each checksum is verified, bad files are rejected.

The data records may appear in any order. For example data at lower
addresses may follow data at higher addresses. Spaces between occupied
memory areas are allowed also. memsim2 will determine the lowest and
highest address at which data is stored and use this information to
auto-detect the simulated chip if no -m option is given.

There are two possible ways to specify the addresses inside the hex
file: seen relative to the storage position inside the memory chip or
absolute inside the system's memory map. For example, think of a 2764
chip of 8 KB size storing code from 0xE000 to 0xFFFF within the system's
memory map of a 6502 system. If seen relative to the storage position
inside the chip, the addresses inside the hex file will start at 0x0000
and rise up to 0x1FFF (8191 decimal). If the addresses inside the hex
file are seen absolute to the memory map, they will start at 0xE000 and
rise up to 0xFFFF.

Either way, relative or absolute addresses, memsim2 will auto-detect
the amount of data and the used addresses to determine the appropriate
chip to simulate.

This detection may however fail if you want to simulate only a subset
of a larger provided rom-set or if there is some unused, reserved space
before the provided data starts.

Remember the 8 / 16 KB example from the binary files where we simulated
only the upper 8 KB? If we had this data provided as hex files with
addresses starting at 0x0000 (relative addresses), simulating only the
upper half would look like this:
```
        memsim2 -m 2764 -o 8192 os16k.hex
```
As another example with absolute addresses inside the hex file, think of
a boot ROM for a 6502 system located at the top of memory, stored in an
8 KB 2764 chip with addresses from 0xE000 to 0xFFFF.

You're working on a monitor code starting at address 0xF000, leaving 4 KB
of free space reserved for further extensions in a range from 0xE000 to
0xEFFF.

memsim2 will notice that the provided data starts at 0xF000 so it will
wrongly assume a start address for the memory chip of 0xF000 which will
put your code in the wrong place: because the simulated chip is visible
from 0xE000 to 0xFFFF, your data will now be placed at 0xE000 instead of
0xF000 and you won't have any useful reset and other system vectors at
the top of memory, thus breaking your system. Just tell memsim2 where the
simulated memory actually starts inside the system's memory map with
the -o option:
```
        memsim2 -m 2764 -o 0xF000 monitor.hex
```
This may sound confusing. As a rule of thumb: if the auto-detection
doesn't work, provide the kind of chip and the starting address within
the system memory map:
```
        memsim2 -m 2764 -o 0xF000 monitor.hex
```
## Motorola S-Record
-----------------

This file format is detected by a couple of file extensions: .s19, .s28,
.s37, .srec and .mot, for example:
```
        memsim2 mydata.s19
```
It doesn't make any difference which actual file extension you provide,
they're all treated in the same manner.

The S-Record file format is very similiar to the Intel Hex file
format, so don't be surprised to find most of the provided description
here just being a copy of the former paragraph describing the Intel Hex
file format. The most notably difference is that a S-Record file may
contain a header with ASCII text that is written to the screen and
can be used to provide a description, version number etc. Another
difference is that there is a provision to detect data corruption if
the actual number of records stored inside the file varies from the
specified number of records. If this should happen, a warning is
written to stderr.

SREC files usually consist of several records (text lines) with
data, each secured with a checksum and provided with an start address
indicating where the following data bytes should get stored.

Each checksum is verified, bad files are rejected.

The data records may appear in any order. For example data at lower
addresses may follow data at higher addresses. Spaces between occupied
memory areas are allowed also. memsim2 will determine the lowest and
highest address at which data is stored and use this information to
auto-detect the simulated chip if no -m option is given.

There are two possible ways to specify the addresses inside the hex
file: seen relative to the storage position inside the memory chip or
absolute inside the system's memory map. For example, think of a 2764
chip of 8 KB size storing code from 0xE000 to 0xFFFF within the system's
memory map of a 6502 system. If seen relative to the storage position
inside the chip, the addresses inside the hex file will start at 0x0000
and rise up to 0x1FFF (8191 decimal). If the addresses inside the hex
file are seen absolute to the memory map, they will start at 0xE000 and
rise up to 0xFFFF.

Either way, relative or absolute addresses, memsim2 will auto-detect
the amount of data and the used addresses to determine the appropriate
chip to simulate.

This detection may however fail if you want to simulate only a subset
of a larger provided rom-set or if there is some unused, reserved space
before the provided data starts.

Remember the 8 / 16 KB example from the binary files where we simulated
only the upper 8 KB? If we had this data provided as hex files with
addresses starting at 0x0000 (relative addresses), simulating only the
upper half would look like this:

```
        memsim2 -m 2764 -o 8192 os16k.s19
```

As another example with absolute addresses inside the hex file, think of
a boot ROM for a 6502 system located at the top of memory, stored in an
8 KB 2764 chip with addresses from 0xE000 to 0xFFFF.

You're working on a monitor code starting at address 0xF000, leaving 4 KB
of free space reserved for further extensions in a range from 0xE000 to
0xEFFF.

memsim2 will notice that the provided data starts at 0xF000 so it will
wrongly assume a start address for the memory chip of 0xF000 which will
put your code in the wrong place: because the simulated chip is visible
from 0xE000 to 0xFFFF, your data will now be placed at 0xE000 instead of
0xF000 and you won't have any useful reset and other system vectors at
the top of memory, thus breaking your system. Just tell memsim2 where the
simulated memory actually starts inside the system's memory map with
the -o option:
```
        memsim2 -m 2764 -o 0xF000 monitor.s19
```
This may sound confusing. As a rule of thumb: if the auto-detection
doesn't work, provide the kind of chip and the starting address within
the system memory map:
```
        memsim2 -m 2764 -o 0xF000 monitor.s19
```

## Specifying the used port
------------------------

The first device is assigned to /dev/ttyUSB0. The digit starts counting
at zero and increments from that on, so that a second simulator would
attach to /dev/ttyUSB1. Please note that this is a generic name used by
the FTDI chip inside the simulator. If you use other equipment with
FTDI chips, these might use the same naming scheme. For details about
how to detect the actual device name refer to the installation section.

On Linux, the device name defaults to /dev/ttyUSB0 so you won't need
to specify it in most cases. If necessary, it may given with the
-d option:
```
        memsim2 -d /dev/ttyUSB1
```

## Configuring reset pulses
------------------------

The memSIM2 USB EPROM simulator comes with a pin connector that
attaches to the reset line. Some systems use high active reset lines,
whereas others use low active ones.

The default configuration for the reset line is low active, with
a reset pulse duration of 200 ms, reset after receiving new data enabled.

The reset configuration is controlled with the -r option. Legal values
are in a range of 1 to 255 milliseconds. Zero disables the reset line.
A positive number issues a reset for high active reset lines, a negative
number issues reset for low active lines.

For example, to configure the reset line for a low active reset line
with a reset pulse duration of 100 ms:
```
        memsim2 -r -100
```

## Memory type
-----------

The -m option controls the memory type to simulate. If this option
is not given, memsim2 tries to auto-detect the most likely chip.
If the option is given but memsim2 thinks it doesn't match the provided
data, it will issue a warning.

| Name  | Size in kilobytes |
|-------|-------------------|
|  2764 |   8 KB            |
| 27128 |  16 KB            |
| 27256 |  32 KB            |
| 27512 |  64 KB            |
| 27010 | 128 KB            |
| 27020 | 256 KB            |
| 27040 | 512 KB            |


## Online help
-----------

If you're in need of tweaking the internal defaults,

```
	memsim2 -h
```

provides this help text with a short description of all available
options:

```
Usage: [OPTION].. FILE
Upload image file to memSIM2 EPROM emulator

Options:
	-d DEVICE     Serial device, defaults to /dev/ttyUSB0
	-m MEMTYPE    Memory type (2764,27128,27256,27512,27010,27020,27040)
	-r RESETTIME  Time of reset pulse in milliseconds.
	              > 0 for positive pulse, < 0 for negative pulse
	-e            Enable emulation
	-o BYTES      Specify an offset value with different meaning for:
	   binary files: skip first n bytes of file
	   Hex files: start address in memory map of simulated memory chip
	-h            This help

Numbers prefixed by '0x' are interpreted as hexadecimal numbers,
octal for numbers beginning with '0' and decimal for everything else.

```

# Features
Basic parameters of the memSIM2 simulator

| Feature | Description |
| --- | --- |
| RAM | 4Mbit (512KB) |
| Simulated Memory | 2764 (8K), 27128 (16K), 27256 (32K), 27512 (64K), 27010 (128K), 27020 (256K), 27040 (512K) |
| Access Time | 70 ns |
| Supply voltage for EPROM | 2.7-5.0 V, the simulator output buffers automatically adapt to the memory supply voltage (device under test must supply Vcc for reference) |
| PC transmission | min. 40 KB / s |
| Simulation cable | Approx. 120 mm long, 28- and 32-pin simulation plugs |
| RESET output | Programmable signal level and pulse duration. It can be activated automatically during transmission. It has a cable with a gripper. |
| | TRANSMISSION (green) - transmission from PC to memSIM |
| LED optical indicators | READY (yellow) - simulator Ready |
| | RUN (red) - output buffers - reading data from memSIM |
| USB cable | USB A-USB B 1.8m long |
| Power | From the control (USB host) computer with complete galvanic isolation |

# EPROM pin out

When I ordered the memSIM2 device I didn't know that the sevice could come with a 28 adapter cable. Now I have to wonder how to hookup the memSIM2 to the device. At this time I'm assuming the only pin that has to be rerouted is the Vcc (power) pin.

[PROM/ROM index and cross reference](https://www.mikesarcade.com/cgi-bin/spies.cgi?action=url&type=info&page=ROMref.txt)

```
(16k)  2k x 8 EPROM (2716)
                              TMS2516  TMS2716 
A7   1 +-v-+ 24  Vcc          +5       +5
A6   2 |   | 23  A8           A8       A8
A5   3 |   | 22  A9           A9       A9
A4   4 |   | 21  Vpp          Vpp      -5
A3   5 |   | 20  /OE          /CE      A10
A2   6 |   | 19  A10          A10      +12
A1   7 |   | 18  /CE          PGM      /CE
A0   8 |   | 17  D7
D0   9 |   | 16  D6
D1  10 |   | 15  D5
D2  11 |   | 14  D4
GND 12 +---+ 13  D3

                                                                 GI9332
                                                                 TMM333
                                                                 TMS4732
(32k)  4k x 8 EPROM (2732)                                       VT2332
                              TMS2516  TMS2708  TMS2716  TMS2532 SY2332 
A7   1 +-v-+ 24  Vcc          +5       +5       +5       +5      +5
A6   2 |   | 23  A8           A8       A8       A8       A8      A8
A5   3 |   | 22  A9           A9       A9       A9       A9      A9
A4   4 |   | 21  A11          Vpp      -5       -5       Vpp     CS2 *
A3   5 |   | 20  /OE Vpp      /CE      /CE      A10      PD/PGM  CS1 *
A2   6 |   | 19  A10          A10      +12      +12      A10     A10
A1   7 |   | 18  /CE          PGM      PGM      CE/      A11     A11
A0   8 |   | 17  D7                                              *polarity of
D0   9 |   | 16  D6                                               chip select
D1  10 |   | 15  D5                                               set at manuf
D2  11 |   | 14  D4
GND 12 +---+ 13  D3

(64k) 8k x 8 EPROM
     JEDEC 2764                    TMS2564
Vpp  1 +-v-+ 28  Vcc         Vpp  1 +-v-+ 28 Vcc 
A12  2 |   | 27  /PGM        /CS1 2 |   | 27 /CS2
A7   3 |   | 26  -           A7   3 |   | 26 Vcc
A6   4 |   | 25  A8          A6   4 |   | 25 A8
A5   5 |   | 24  A9          A5   5 |   | 24 A9
A4   6 |   | 23  A11         A4   6 |   | 23 A12
A3   7 |   | 22  /OE         A3   7 |   | 22 PD /PGM
A2   8 |   | 21  A10         A2   8 |   | 21 A10
A1   9 |   | 20  /CE         A1   9 |   | 20 A11
A0  10 |   | 19  D7          A0  10 |   | 19 D7
D0  11 |   | 18  D6          D0  11 |   | 18 D6
D1  12 |   | 17  D5          D1  12 |   | 17 D5
D2  13 |   | 16  D4          D2  13 |   | 16 D4
GND 14 +---+ 15  D3          GND 14 +---+ 15 D3

(128k) 16k x 8 EPROM
       27128
Vpp  1 +-v-+ 28  Vcc
A12  2 |   | 27  /PGM
A7   3 |   | 26  A13
A6   4 |   | 25  A8
A5   5 |   | 24  A9
A4   6 |   | 23  A11
A3   7 |   | 22  /OE
A2   8 |   | 21  A10
A1   9 |   | 20  /CE
A0  10 |   | 19  D7
D0  11 |   | 18  D6
D1  12 |   | 17  D5
D2  13 |   | 16  D4
GND 14 |   | 15  D3
       +---+

(256k) 32k x 8 EPROM
       27256
Vpp  1 +-v-+ 28  Vcc
A12  2 |   | 27  A14
A7   3 |   | 26  A13
A6   4 |   | 25  A8
A5   5 |   | 24  A9
A4   6 |   | 23  A11
A3   7 |   | 22  /OE
A2   8 |   | 21  A10
A1   9 |   | 20  /CE /PGM
A0  10 |   | 19  D7
D0  11 |   | 18  D6
D1  12 |   | 17  D5
D2  13 |   | 16  D4
GND 14 +---+ 15  D3


(512k) 64k x 8 EPROM
       27512

A15  1 +-v-+ 28 Vcc
A12  2 |   | 27 A14
A7   3 |   | 26 A13
A6   4 |   | 25 A8
A5   5 |   | 24 A9
A4   6 |   | 23 A11
A3   7 |   | 22 /OE
A2   8 |   | 21 A10
A1   9 |   | 20 /CE
A0  10 |   | 19 D7
D0  11 |   | 18 D6
D1  12 |   | 17 D5
D2  13 |   | 16 D4
GND 14 +---+ 15 D3

 JEDEC 128k x 8 EPROM 27C010

Vpp  1 +-v-+ 32  Vcc
A16  2 |   | 31  /pgm
A15  3 |   | 30  nc
A12  4 |   | 29  A14
A7   5 |   | 28  A13
A6   6 |   | 27  A8
A5   7 |   | 26  A9
A4   8 |   | 25  A11
A3   9 |   | 24  /OE
A2  10 |   | 23  A10
A1  11 |   | 22  /CE
A0  12 |   | 21  D7
D0  13 |   | 20  D6
D1  14 |   | 19  D5
D2  15 |   | 18  D4
GND 16 +---+ 17  D3

JEDEC 27020 EPROM

VPP  1 +-v-+ 32  Vcc
A16  2 |   | 31  /PGM
A15  3 |   | 30  A17
A12  4 |   | 29  A14
A7   5 |   | 28  A13
A6   6 |   | 27  A8
A5   7 |   | 26  A9
A4   8 |   | 25  A11
A3   9 |   | 24  /OE
A2  10 |   | 23  A10
A1  11 |   | 22  /CE
A0  12 |   | 21  D7
D0  13 |   | 20  D6
D1  14 |   | 19  D5
D2  15 |   | 18  D4
GND 16 +---+ 17  D3

      27C040

VPP  1 +-v-+ 32  +5V
A16  2 |   | 31  A18
A15  3 |   | 30  A17
A12  4 |   | 29  A14
A7   5 |   | 28  A13
A6   6 |   | 27  A8
A5   7 |   | 26  A9
A4   8 |   | 25  A11
A3   9 |   | 24  /OE
A2  10 |   | 23  A10
A1  11 |   | 22  /CE /PGM
A0  12 |   | 21  D7
D0  13 |   | 20  D6
D1  14 |   | 19  D5 
D2  15 |   | 18  D4
GND 16 +---+ 17  D3

```
## Adapters

I haven't tried this yet but I expect this will work.

```
      27C040                   64k x 8 EPROM               2k x 8 EPROM
                    
Vpp  1 +-v-+ 32  Vcc               27512                       2716                   TMS2516  TMS2716 
A16  2 |   | 31  A18                                                                           DON'T USE
A15  3 |   | 30  A17        A15  1 +-v-+ 28  Vcc  ........................  --+                THIS!
A12  4 |   | 29  A14        A12  2 |   | 27  A14                              | Jumper
A7   5 |   | 28  A13        A7   3 |   | 26  A13        A7   1 +-v-+ 24  Vcc--+       +5       +5
A6   6 |   | 27  A8         A6   4 |   | 25  A8         A6   2 |   | 23  A8           A8       A8
A5   7 |   | 26  A9         A5   5 |   | 24  A9         A5   3 |   | 22  A9           A9       A9
A4   8 |   | 25  A11        A4   6 |   | 23  A11        A4   4 |   | 21  Vpp          Vpp      -5  <- ***
A3   9 |   | 24  /OE        A3   7 |   | 22  /OE        A3   5 |   | 20  OE/          /CE      A10
A2  10 |   | 23  A10        A2   8 |   | 21  A10        A2   6 |   | 19  A10          A10      +12 <- ***
A1  11 |   | 22  /CE /PGM   A1   9 |   | 20  /CE /PGM   A1   7 |   | 18  CE/          PGM      /CE
A0  12 |   | 21  D7         A0  10 |   | 19  D7         A0   8 |   | 17  D7
D0  13 |   | 20  D6         D0  11 |   | 18  D6         D0   9 |   | 16  D6
D1  14 |   | 19  D5         D1  12 |   | 17  D5         D1  10 |   | 15  D5
D2  15 |   | 18  D4         D2  13 |   | 16  D4         D2  11 |   | 14  D4
GND 16 +---+ 17  D3         GND 14 +---+ 15  D3         GND 12 +---+ 13  D3

```

Copyright
---------

Written by Simon Berg <ksb@fluffware.se>, Nils Eilers <nils.eilers@gmx.de> and Neil Cherry <ncherry@linuxha.ccom>
memSIM2 is free software, licensed under GPL v2 or later
