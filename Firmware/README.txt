What: Firmware for Dr. Squiggles Project
Language: C and C++
Hardware: Teensy 3.2 (PJRC)

Installation Instructions:
1) Install Arduino (1.8.9) https://www.arduino.cc/en/Main/Software
2) Install and run Teensyduino: https://www.pjrc.com/teensy/td_download.html
3) Copy the files from ./Teensy_Replacements to /Applications/Arduino.app/Contents/Java/hardware/teensy/avr/cores/teensy3/usb_desc.h, overwriting existing versions of those files.
4) All files from ../Robot_Communication_Framework need to be symlinked into this folder, e.g. with ln -s ../Robot_Communication_Framework/* ./
5) Compile in Arduino with Board: Teensy 3.2 and USB Type "MIDI"
6) Say like 10 hail-marys, if you are in to that type of thing.