## Compiling

This code is meant to be compiled in the Arduino environment with the addition of the 
Raspberry Pi Pico RP2040/RP2350 board package by Earle F. Philhower, III.
It works on both the Pi Pico and Pi Pico 2 boards.

## The Sportident autosend protocol

The code in SerialInput.cpp handles both the legacy and extended Sportident protocols 
for autosend data. Other kinds of Sportident packages are forwarded as well, but that 
is not thought to be a valid use case, nor is it tested much.

Each packet contains a 16-bit CRC. This is not checked. Packets are passed on regardless 
if the CRC is valid or not.

## Serial ports

PIO/Software serial ports are used throughout. Documentation for the serial port library 
can be found at [Readthedocs](https://arduino-pico.readthedocs.io/en/latest/piouart.html)

Input is taken from up to five serial ports. A standard way of building the box is to have 
four DB9M inputs and one (opto-isolated) long-wire input, but one can e.g. build it with just 
three long-wire inputs. In the latter case, it is suggested that the line:

    #define RECEIVE_ONLY 0

In SI-concentrator.ino is changed to:

    #define RECEIVE_ONLY 1

This changes the printouts on the LCD to only show status of the three ports and defaults them 
all to 4800 baud, which is the speed used on the long wire.

Initially, if an invalid packet is received, the serial speed is toggled between 
4800 baud and 38400 baud. Once a valid packet has been received, the speed is locked 
for that input. Except for the long wire input, the starting speed is 38400.

The output is sent to three places:

1. The female DB9 connector
2. The long-wire output (supports at least 1 km of DL-1000 wire)
3. The USB serial port

## Link status

The (optional) 2x8 LCD displays the status of the different inputs as well as the battery
voltage. There is one character position for each of the five inputs. The possible status 
characters and their meanings are:

'?' - Nothing has been received  
'f' - An invalid packet was received  
't' - There was a timeout while recieving a packet  
'u' - A valid packet of an unexpected type was received  
'tick' - A perfectly valid extended protocol autosend packet was recevied  
'L/E' - A perfectly valid legacy protocol autosend packet was recevied

The status display toggles between the above characeters and another character showing the
link speed for each input:

'3' - 38400 baud  
'4' - 4800 baud

As soon as a valid packet has been received, the backlight is turned on and the card number
as well as the station code is shown for a few seconds. This greatly helps in verifying the
operation of the setup.

## Powerbank keep-alive

A common way of powering the concentrator box is from a USB power bank via the USB cable.
The power consumption is however so low that most power banks think that no load is 
connected, so the power bank switches off its output after 30 seconds or so. To prevent
this, there is a software-controlled feature to periodically (every 25 seconds) draw 
extra current for 1.1 seconds. This prevents the power bank from going to sleep and 
powering down the concentrator box.

It might be necessary to tweak the timing of the current pulses to fit with some specific 
USB power bank. This is done by the following lines in the .ino file:

    static const uint32_t POWER_BANK_PULSE_MS = 1100;         // Length of power bank keep-alive pulse
    static const uint32_t POWER_BANK_PULSE_PERIOD_MS = 25000; // Time between power bank keep-alive pulses

## Debugging

To enable debug printouts, a line in the file debug.h should be changed from:

    #define DEBUG 0
  
to

    #define DEBUG 1
  
This sends debug printouts to the USB serial port. This obviously breaks the Sporitdent 
protocol on that port.
