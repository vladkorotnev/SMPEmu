# Genjitsu SMP
## The flash cartridge for the Elektronika MK90 calculator

-----

Current stable revision: 0.2.1 at 04 May 2018

-----

Tired of having to use a programmer to upload programs onto your MK90?
Want to have more than one program on one SMP? 

-> This is the project you've been waiting for!
	
Concept video: [Youtube](https://www.youtube.com/watch?v=kwWYTsK77sc) (on fork of the MK90 emulator with extended SMP command set)

Photo in action: ![photo](https://pp.userapi.com/c834201/v834201308/9c322/eeXcRKbbKVo.jpg)

## Disclaimer
 This project is provided as-is and may require sufficient electronics skills to build it properly. Nobody of the authors or contributors to this project may be held responsible for any damage or loss due to any use of this project.

## Current status:

### Hardware-wise

* Based on the ATMEGA328 via an Arduino (e.g. Uno, Nano)
* Boots fine from the Arduino program memory
* Needs no overclocking done to the arduino

### Software-wise:
	
* The MK90 image chooser side program boots fine on an emulator, and as of May 2018, allows to input a file name
* Uses it's own protocol not conflicting with the MK90 SMP command set (probably may be changed in the future)

## Roadmap

* Integrate the SD-FAT library to access a memory card with SMP images stored on it
* Move/fork to the ESP32 platform if there isn't enough speed in the ATMega, or to add WiFi/Bluetooth capability for internet access (I want to chat online via the MK90!)

## Schematic:
	 (see http://www.pisi.com.pl/piotr433/mk90cahe.htm for reference)
	 Pin 2 Vcc of calculator into +5V of Arduino (ONLY when PC is not connected to arduino!)
	 Pin 3 CLK of calculator into pin 4 via R=220 Ohm 
	 Pin 4 DAT of calculator into pin 5 via R=270 Ohm
	 Pin 5 SEL of calculator into pin 3 of Arduino 
	 Pin 6 GND of calculator into GND of Arduino

## Thanks to
	
* Piotr Piatek for the extensive info on the calculator ([Link](http://pisi.com.pl/piotr433/index.htm#mk90), I also made a mirror in case it goes down: [Genjitsu Labs Mirror](http://piotr433.archives.genjit.su/index.htm#mk90) )
* Kirill Leyfer ([YouTube](https://youtube.com/user/kogdazjasdohnu)) for helpful debugging and testing sessions during exam eve nights
* Everyone else for your support 

## Stay tuned

* [Genjitsu Labs](http://genjit.su)
* [VK](http://vk.com/akasaka)
* [Twitter](http://twitter.com/vladkorotnev)
