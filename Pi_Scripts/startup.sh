#! /bin/sh

cd /home/pi/Dr_Squiggles/
git pull
mkdir /home/pi/Dr_Squiggles/Beat-and-Tempo-Tracking/
cd /home/pi/Dr_Squiggles/Beat-and-Tempo-Tracking/
git pull
sudo chmod a+x /home/pi/Dr_Squiggles/Pi_Scripts/startup.sh
cd /home/pi/Dr_Squiggles/Main
sudo apt-get install libasound2-dev
gcc sq.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o sq
sudo cp ./sq /usr/local/bin/sq
gcc op.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o op
sudo cp ./op /usr/local/bin/op
gcc opc.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o opc
sudo cp ./opc /usr/local/bin/opc
gcc op2.c core/*.c ../Robot_Communication_Framework/*.c ../Beat-and-Tempo-Tracking/src/*.c Rhythm_Generators/*.c extras/*.c -lasound -lm -lpthread -lrt -O2 -o op2
sudo cp ./op2 /usr/local/bin/op2
gcc cmd.c extras/*.c -lm -lpthread -o cmd
sudo cp ./cmd /usr/local/bin/cmd
#/home/pi/Dr_Squiggles/Main/a.out
