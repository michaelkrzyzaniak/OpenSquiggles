#! /bin/sh

cd /home/pi/Dr_Squiggles/
git pull
cd /home/pi/Dr_Squiggles/Beat-and-Tempo-Tracking/
git pull
cd /home/pi/Dr_Squiggles/Main
sudo apt-get install libasound2-dev
gcc *.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c -lasound -lm -lpthread -O2
#/home/pi/Dr_Squiggles/Main/a.out
