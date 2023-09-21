This project contains the following targets:

project.mcp (for CW7.2):

* CPUStick 5211    -- skeleton code to run on an MCF5211 (source code)
* CPUStick 52221   -- skeleton code to run on an MCF52221, with USB host
                      and device drivers (source code)
* CPUStick 52233   -- skeleton code to run on an MCF52233, with Niche Lite
                      TCP/IP drivers (binary form)
* CPUStick 52259   -- skeleton code to run on an MCF52259, with USB host
                      and device drivers (source code)
* CPUStick DemoKit -- skeleton code to run on an MCF52259, under the USB
                      MST bootloader, with USB host and device drivers
                      (source code)
* Flasher (RAM)    -- RAM target to clone firmware to another MCU via
                      QSPI/EzPort
* Empty            -- empty project to hold places for header files, etc.,
                      outside of Link Order

flexis.mcp (for CW6.2):

* CPUStick 51cn128   -- skeleton code to run on an MCF51CN128 (source code)
* CPUStick 51jm128   -- skeleton code to run on an MCF51JM128, with USB host
                        and device drivers (source code)
* CPUStick Badge     -- skeleton code to run on a Badge Board, under the USB
                        MST bootloader, with USB host and device drivers
                        (source code)
* CPUStick 51qe128   -- skeleton code to run on an MCF51QE128 (source code)
* CPUStick 9s08qe128 -- skeleton code to run on an MC9s08qe128 (source code)

hcs12.mcp (for CW 4.7)

* CPUStick 9s12dt256 -- skeleton code to run on an MC9s12dt259 (source code)
* CPUStick 9s12dp512 -- skeleton code to run on an MC9s12dp512 (source code)

pic32.mcw (for MPLAB8.53):

* pic32.mcp          -- skeleton code to run on a PIC32MX3/4, with USB host
                        and device drivers (source code)
                    
Note that the remaining targets are private, with no source code provided.

The StickOS library is provided in binary form, and can be removed from
the project for a true skeleton without StickOS functionality.

The Niche Lite TCP/IP stack is provided in binary form to make the skeleton
code stand alone.  If you have the v6.4 headers (available from Freescale),
you can use other features other than just accepting a raw connection for
the command-line.

The skeleton code provides a rudimentary command-line interface for the
MCU.  For the MCF52259, MCF52221, MCF51JM128, and PIC32MX4, this is provided
by a USB device driver and an upper level CDC/ACM device class driver that
connects to a USB Virtual COM Port on the host PC.  For the MCF52233, this is
provided by the Niche Lite TCP/IP stack that accepts a raw connection from
a socket on the host PC on port 1234.  All MCUs also support a physical UART
connection on their lowest numbered UART.

The skeleton code also provides the following features, identical to those
found in StickOS BASIC, allowing easy porting of BASIC programs to C:

* remote wireless command-line over zigbee via MC1320x transceiver
* upgrade firmware via the terminal command-line
* clone firmware to another MCU via QSPI/EzPort
* a host mode USB driver and MST class driver to connect to USB devices
  when not using the command-line (MCF52221 only)
* programmatic access to flash memory; optional flash security
* MCU pin manipulation routines for digital, analog, frequency I/O, uart
* lightweight printf to either terminal or CW debug console
* boot-time detection of debugger presence
* programmable interval timer, a/d converter, sleep mode example code

To run the Skeleton USB code, build the bits and flash the board.  Save
cpustick.inf to a file and right-click -> Install.  Connect the board.
Let Windows auto-install the drivers.  Open HyperTerminal and connect
to the new Virtual Com Port.  Press <Enter> for a prompt (this may take
a few seconds).

To run the Skeleton TCP/IP code, build the bits and flash the board.
Connect the Ethernet connector on the board to your router.  Let
the board get an IP address from DHCP (query your DHCP server to figure
out which IP address it got).  Open HyperTerminal and connect to port
1234 of the obtained IP address.  Press <Enter> for a prompt (this may
take a few seconds).  You can then use the "ipaddress" command to set a
static IP address; you can override the static IP address and revert to
DHCP by holding SW2 depressed during boot.

To run the skeleton UART code, build the bits and flash the board.
Connect the lowest numbered UART to the host PC, potentially with a level
shifter.  Set the PC to 9600 baud, 8 data bits, no parity, and XON/XOFF
flow control.  Open HyperTerminal and connect to the Physical Com Port.
Press <Enter> for a prompt (this may take a few seconds).

If you have an IOStick or a 1320xRFC RF daughter card, you can use
zigbee to connect from one node to another -- just set the nodeid with
the "nodeid" command and then use the "connect" command to connect to
the remote node.

The code starts at startup.c which calls its own init().  For most MCU
bits, it continues to main.c.  For the MCF52233 bits, on the other hand,
it continues to the Niche Lite code which starts a tasking system, and
then resumes to main.c.  From there code continues to the project-specific
skeleton.c.

The general purpose sources files are as follows:

adc.[ch]       -- simple a/d converter driver
clone.[ch]     -- QSPI/EzPort CPU-to-CPU flash cloner
flash.[ch]     -- flash access and CDC/ACM-based USB upgrade routines
ftdi.[ch]      -- CDC/ACM device class driver
i2c.[ch]       -- i2c transport driver
led.[ch]       -- trivial LED status driver
pin.[ch]       -- MCU I/O pin manipulation
printf.[ch]    -- lightweight printf to terminal or CW debug console
qspi.[ch]      -- qspi transport driver
scsi.[ch]      -- mst class host controller driver
sleep.[ch]     -- sleep mode driver
terminal.[ch]  -- vt100 terminal emulator driver (on CDC/ACM or tcp/ip)
timer.[ch]     -- simple programmable timer interrupt driver
usb.[ch]       -- dual mode host controller/device driver
util.[ch]      -- basic utility routines
zigbee.[ch]    -- zigbee wireless transport over qspi

Once you install the real Freescale headers (MCF52221.h, MCF52235.h, etc.,
and their descendants) in your headers directory, you can turn off the
#define EXTRACT in config.h to begin using them.
