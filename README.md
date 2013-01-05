avr-heating
===========
Simple at2313 heating control system based around a USB relay control board
available at:

http://www.sigma-shop.com/product/67/usb-relay-controller-one-channel-pcb.html

The board by itself allows toggling of a relay via USB. I thought it might be fun
to try my hand at a bit of MCU programming and allow it to function autonomously
using a timed schedule.

To control the heating in my home, I by-passed an existing wall-mounted
thermostat (which is just temperature controlled switch) and connected the
wires to the board's mains relay.


Dependencies
------------
The following packages are needed on Ubuntu 12.10:
* ruby1.8
* gcc-avr
* avr-libc
* avrdude


Build/Use
---------
To program the board, I had to replace the existing MCU chip as it seemed to
have been locked in some way to prevent programming. I never completely
understood by what mechanism though as device flags made it look unlocked.
Maybe this isn't necessary but a new chip was cheap and easy to come by.

To program the device I left the MCU in situ on the board and wedged
jumper-wires between the pins and the PCB socket. I then connected these
directly to a parallel-port header on a PC motherboard. At the same time I left
the board connected to the USB socket for power. Crude, but seems to work OK.

The MCU control program can be built and uploaded to the device with the
following commands:
	make
	make burn

Once programmed only the USB connection is necessary (although the programming
wires can be left in place if desired). The 'ctrl.rb' script is used to set-up
the timer/get status etc. When started with no arguments it prints a list of
possible options.

The script 'setStandard.sh' sets a simple schedule.

In order to be useful, the USB port needs to remain powered even if the
attached PC is turned off. I found I could enable this by playing with BIOS
settings relating to wake-on-LAN over USB. This could probably be achieved
through other means such as a powered USB hub etc.

The clock used to control the timer is likely to drift a bit, so I recommend
putting something like:

	(sleep 30 ; /home/someuser/avr-heating/ctrl.rb syncTime) &

In /etc/rc.local.

As it can be a bit of a pain to use a computer to quickly turn the heating
on/off, a push-button can be added between pins 10 (GND) and (PD6). Each
push/release toggles the on/off state of the relay.


Known Issues
------------
* My implementation of the USB-UART command rx/tx doesn't seem to be completely
  reliable hence there are a few sleeps in ctrl.rb. This could probably be
  fixed by someone a bit more experienced with AVR programming.


Licensing
---------
It probably would have been easier to have the original source to the MCU on
the relay board to work with. Unfortunately the manufacturer declined to release
it to me and I was forced to write this from scratch.

To ensure others in future are not stuck in a similar position I am making this
source available publicly under revision 3 of the GPL.  Details can be found in
the LICENSE file.
