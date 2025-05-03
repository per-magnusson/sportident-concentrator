# Sportident Concentrator Box

This project consists of both the hardware and the software for a Sporitdent  
concentrator box based on a Raspberry Pi Pico.

<img src="https://github.com/user-attachments/assets/3072d64d-388f-45a6-818a-dd3b3e6587b5" alt="photo of the concentrator" width="600"/>

It is useful in the sport of orienteering where the Sporitdent punching system is used and allows
punch data from the terrain to be brought in real time to the speaker at the event and to online results. The box
can receive punch data from up to four BSF7 RS232 statations (or other concentrator boxes), plus from a 
long wire from another concentrator box. It buffers the punch data and sends it out over:

- A long wire (1 km or even more is supported)  
- A USB port to a computer  
- An RS232 serial port  

It can be powered by a USB power bank, from a computer via the USB cable or from a pack of e.g. three AA
batteries or another source that provides between 1.8 and 5.5 V.

The box supports both the old Sportident legacy protocal and the current 
extended protocol. It adapts to the speed on the serial port which can be either
4800 or 38400 baud.

It is possible to build a version (useful e.g. at the event center) on the same PCB which has up to three opto-isolated 
long-wire inputs (and correcpondingly fewer RS232 inputs) to receive data from three different long wires.

Since the box in its normal configuration has a long-wire input, it is possible to daisy-chain a number of
concentrator boxes from e.g. the pre-warning via the last control.

To significantly simplify verification of the setup and troubleshooting in the field, there is a 2x8 character 
display that constinuously shows the status of the various inputs. Right after a packet of punch data is 
received, the display shows the card number and station code. 

The USB port shows up as a serial adapter and the punch data can be received by MeOS, OLA and
other orienteering administration software.

The design is based on a Raspeberry Pi Pico (both the original and Pi Pico 2 works) and the code
can be compiled in the Arduino environment.

The project contains the source code, files for manufacturing and assmebling the PCB and
files for 3D-printing the enclosure.
