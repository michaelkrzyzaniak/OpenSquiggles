#! /bin/sh

mkdir /home/pi/Dr_Squiggles/
cd /home/pi/Dr_Squiggles/
git pull
mkdir /home/pi/Dr_Squiggles/Beat-and-Tempo-Tracking/
cd /home/pi/Dr_Squiggles/Beat-and-Tempo-Tracking/
git pull
sudo chmod a+x /home/pi/Dr_Squiggles/Pi_Scripts/startup.sh
cd /home/pi/Dr_Squiggles/Main
sudo apt-get install libasound2-dev
gcc *.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2
#/home/pi/Dr_Squiggles/Main/a.out
