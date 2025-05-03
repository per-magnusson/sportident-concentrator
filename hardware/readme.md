# Layout

The board looks something like this:

<img src="https://github.com/user-attachments/assets/4ccc6f21-f926-4c1b-a432-c2942fb5094f" alt="PCB Rendering" width="600"/>

The board has two layers, where most of the routing and the components are on the top side. The 
bottom side is mostly ground plane. Here are the layers:

<img src="https://github.com/user-attachments/assets/7ca29294-1ef9-439e-9bff-43012ccb6509" alt="Top layout" width="300"/>
<img src="https://github.com/user-attachments/assets/0b9e2481-0769-48ce-9101-6f726632f7a8" alt="Bottom layout" width="300"/>

Here is the assembly drawing:

<img src="https://github.com/user-attachments/assets/39de60f4-e358-493e-8203-88676799b689" alt="Assembly drawing" width="600"/>

Vectorized versions are avaialble as PDFs in the repo.

Gerber and assembly files are available in the zip files in the repo. The PCB file is the gerbers for the PCB. 
The PCA file contains what is needed for assembly.

The bill of materials (BOM) should be self-explanatory.

# Schematic overview

The schematic is available as a single-page PDF in the repo.

<img src="https://github.com/user-attachments/assets/083c931d-f579-4d9e-bdfe-268bfe7c2449" alt="Assembly drawing" width="600"/>

## Notation

Components marked "NM" are normally Not Mounted,

## Processor

At the core of the deisgn is a Raspberry Pi Pico 2 board (Pi Pico also works if the code is compiled for that). It 
contains the processor, a USB port and a buck/boost voltage converter. Using this inexpensive board simplifies the rest
of the design so that it should be rather easy to hand-solder for most people with at least a little soldering
experience.

## Power

Power normally comes via the USB cable, either from a power bank or from a computer. There are also provisions to power 
the board from a battery pack (via P5) with a voltage between 1.8 and 5.5 V. E.g. using three AA batteries. The power 
switch SW1 only has an effect if using the battery pack option. Power via the USB port always turns the board on.

Q4 disconnects the battery if power is available from the USB port.

## Serial ports

There are three DB9 connectors directly on the board: two male inputs to be connected to Sportident BSF7 RS232 statations and
one female output to be connected to a serial port on a computer or to another concentrator box. Two more input ports
can easily be added to TP10/TP11 and TP12/TP13, but the DSUB connectors need to be connected to these points by wire.
Alternatively, one or both of these inputs can be configured for long-wire reception by mounting U5/R21 and/or U6/R22 
instead of Q1 and/or Q5.

Only one "proper" RS232 line driver/receiver is used (U1) as there is only one RS232 output. The inputs can be handled
in a simpler and cheaper way by discrete solutions using a MOSFET and a handful of resistors (and ESD protection diodes).

The optocoupler U2 (connected to TP8/TP9) allows for an additional long-wire input from another concentrator box. 

There is also a long-wire output, built around Q7/Q3. It transmits the same data as that transmitted on the serial port
outputs. It is best to use 4800 baud on the long wires as that ensures that reflections are not a problem. Maybe 38400
might work, but that has not been tested and the data volumes in any reasonable scenario are so low that 4800 will do 
just fine. An LED, D1, flashes along with the data sent on the long wire output, which might be helpful during debugging,
particularly if the LCD is not fitted. With the LCD in the design, the LED could be left out without any real loss in 
debuggability.

At least 1 km of DL 1000 cable can be used as the long wire.

If the USB cable is connected to a computer, it shows up as a serial port and the punch data can be received 
by most orienteering administrative software, like OLA and MeOS without any need for a separate RS232-to-USB adapter.

## LCD

The 2x8 character LCD U4 allows for monitoring the status of the box and the communication. This greatly simplifies 
troubleshooting and verification of the setup in the field. However, if it is very important to keep the cost down,
the LCD can be left out without having to modify the software.

The LCD contrast can be adjusted by R18, or if the ideal value is known, the fixed R19 can be mounted instead.

The backlight of the LCD can be controlled by Q6.

The LCD can be mounted on 3D-printed spacers and connects to the main board via a 2x7 pin header.

## Power bank keep-alive

A common way of powering the concentrator box is from a USB power bank via the USB cable. The power consumption 
is however so low that most power banks think that no load is connected, so the power bank switches off its 
output after 30 seconds or so. To prevent this, Q2/R3/R4 can be used to periodically pull more current from
the source to prevent the power bank from going to sleep.

## Config switches

There are provisions for mounting a four-pole DIP switch (SW2) and a push button (SW3). These could be used for 
configuring the device in various ways, but the current software does not read these switches. Accessing
them would requre disassembling the box, which is a bit inconvenient in the field as it requires unscrewing four 
screws. It is probably best to leave these switches unpopulated.
best to not fit them. Configuration could of course also be done by changing and recompiling the code.
