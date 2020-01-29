import time
import rtmidi
import random
import copy
import argparse
from pythonosc import dispatcher as DPatcher
from pythonosc import osc_server

from pythonosc import udp_client
import math

midiout = rtmidi.MidiOut()
midiout2 = rtmidi.MidiOut()
available_ports = midiout.get_ports()

def beat_handler(unused_addr,args,timestamp, robot_id, beat_id):
	print(args)
	timestamp,robot_id,beat_id
	print(timestamp, robot_id, beat_id)


def print_volume_handler(unused_addr, args, volume):
	print("[{0}] ~ {1}".format(args[0], volume))

def print_compute_handler(unused_addr, args, volume):
	try:
		print("[{0}] ~ {1}".format(args[0], args[1](volume)))
	except ValueError: pass


beatid = 0
c_time = 0

class Robot:
	def __init__(self,ID):
		self.time = 0
		self.id = ID


class OSC_handler:
	def __init__(self):
		r_times = []

		self.robots = []
		self.robots.append(Robot(1))
		parser = argparse.ArgumentParser()
		robotTestIP = "192.168.43.116"
		robotTestIP = "127.0.0.1"
		robotTestIP = "192.168.43.135"
		parser.add_argument("--ip", default=robotTestIP,
					 help="The ip to listen on")
		parser.add_argument("--port", type=int, default =9000,
					  help="The port to listen on")

		broadcastIp = "255.255.255.255"
		parser.add_argument("--bcip", default=broadcastIp,
					 help="The ip to broadcast on")
		parser.add_argument("--bcport", type=int, default =9001)
		args = parser.parse_args()
		self.client = udp_client.SimpleUDPClient(args.bcip, args.bcport, True)


		dispatcher = DPatcher.Dispatcher()
		#dispatcher.map("/beat", beat_handler, "beat")
		dispatcher.map("/beat", self.broadcast, "rhythm")
		#dispatcher.map("/volume", print_volume_handler, "Volume")
		#dispatcher.map("/logvolume", print_compute_handler, "Log volume", math.log)
		
		self.server = osc_server.ThreadingOSCUDPServer((args.ip,args.port),dispatcher)
		print("Serving on {}".format(self.server.server_address))
		self.server.serve_forever()

		c_parser = argparse.ArgumentParser()
		c_parser.add_argument("--ip", default="192.168.43.135",
			help="The ip of the OSC server")
		c_parser.add_argument("--port", type=int, default=9001,
			help="The port the OSC server is listening on")
		c_args = parser.parse_args()
		self.client = udp_client.SimpleUDPClient(c_args.ip, c_args.port)
	def broadcast(self,unused_addr, args,timestamp, robot_id, beat_id):
		print("BC")
		msg = [robot_id,beat_id,0,-1,-1]
		self.client.send_message("/rhythm", msg)

	def update(self):
		# receive message TODO

		# send message
		beatid+=1
		self.client.send_message("/rhythm", self.robots[0].id, beatid, 100.0,20)




if available_ports:
	#midiout.open_port(2)
	midiout2.open_port(1)
else:
	midiout.open_virtual_port("Virtual Port")

style = "major"
# pentatonic
bassnotelib = [37,39,42,44,46]
midinotelib = [49,56,61,63,66,68,70,73,75,78,80,82]
if style == "major":
	# major
	bassnotelib = [36,38,40,41,43,45]
	midinotelib = [48,55,60,62,64,65,67,69,71,72,74,76,77,79,81]
elif style == "blues":
	bassnotelib = [36,43,41,39,38]
	midinotelib = [48,55,60,62,63,65,66,67,70,72,75,78,79]
elif style == "minor":
	bassnotelib = [36,43,41,39,38]
	midinotelib = [48,55,60,62,63,65,67,70,72,75,79]


#squiggleslib = [62,63,64,65,66,67,59,68,69,70,71]
squiggleslib = [62,63,64,65,66,67]
#squiggleslib = [59]
#squiggleslib = [38,39,40,41,42,43,44,45,46,47,50,51,52,53,54,55,56,57,58,59,62,63,64,65,66,67,59,68,69,70,71]
#squiggleslib = [60]
squigglesbasslib = [60,61]

bpm = 120
#sequences per beat
spb = 4
notedensity = 0.6
bnotedensity = 0.8
playSquiggles = True
mutrate = 0.2
maxSustain = 32
sequenceLength = 32
volume = 127
#volumevalues = [127,100,80,60,20]
volumevalues = [127]
notesOn = []
notesOn2 = []

def playnotes (m_out, n_on):
	for n in n_on:
		if (n.repeat == False and n.state == 0):
			playnote(m_out,n.note, n.vel)
		n.state+=1
		if (n.state >= n.sus):
			# switch note off
			offnote(m_out,n.note)
			# remove the note from the notesOn list
			n_on.remove(n)


class Sequence:
	def __init__(self, l,lib, notedensity):
		self.length = l
		self.notes = []
		self.lib = lib
		self.state = 0
		for i in range(l):
			if random.uniform(0.0,1.0) < notedensity:
				self.notes.append(Note(random.choice(lib), random.randint(2,2+maxSustain)))
			else:
				self.notes.append(Note(0, random.randint(2,2)))
	def play(self, interval, n_on):
		if (self.state % interval == 0 or self.state==0):
			notenr = (int)(self.state/interval)
			if self.notes[notenr].note > 0:
				n_on.append(copy.deepcopy(self.notes[notenr]))
				# playnote(self.notes[notenr].note)
		self.state+=1
		if self.state >= self.length:
			self.state = 0
	def mutate(self,mutrate, notedensity):
		for i, n in enumerate(self.notes):
			if random.uniform(0.0,1.0) < mutrate:
				if random.uniform(0.0,1.0) < notedensity:
					self.notes[i] = Note(random.choice(self.lib), random.randint(2,2+maxSustain))
				else:
					self.notes[i] = Note(0, random.randint(2,2))


class Note:
	def __init__(self, note, sustain):
		self.note = note
		self.repeat = False
		self.sus = sustain # sustain
		self.state = 0
		self.vel = random.choice(volumevalues)

def playnote(m_out,note, vel):
	note_on=[0x90,note,vel] 
	# print("note_on")
	m_out.send_message(note_on)

def offnote(m_out,note):
	note_off=[0x80,note,0] 
	#print("note_off")
	m_out.send_message(note_off)

def main():
	with midiout:
		play = True
		seqcounter = 0
		mutinterval = sequenceLength
		seq = Sequence(sequenceLength,midinotelib,notedensity)
		bseq = Sequence(sequenceLength,bassnotelib,bnotedensity)
		squiqseq = Sequence(sequenceLength,squiggleslib, notedensity)
		squiqseq2 = Sequence(sequenceLength,squiggleslib, notedensity)
		squiqbseq = Sequence(sequenceLength,squigglesbasslib,bnotedensity)
		while play:
			if playSquiggles:
				squiqseq.play(1,notesOn)
				squiqseq2.play(1,notesOn)
				squiqbseq.play(spb,notesOn)
				playnotes(midiout,notesOn)
			#else:
				seq.play(1,notesOn2)
				bseq.play(spb,notesOn2)
				playnotes(midiout2,notesOn2)
			hz = 60/bpm/spb
			time.sleep(hz)
			#print(len(notesOn))
			seqcounter+=1
			if (seqcounter % mutinterval == 0):
				print("Mutating with " ,mutrate, " probability")
				if playSquiggles:
					squiqseq.mutate(mutrate, notedensity)
					squiqseq2.mutate(mutrate, notedensity)
					squiqbseq.mutate(mutrate, bnotedensity)
				#else:
					seq.mutate(mutrate,notedensity)
					bseq.mutate(mutrate,bnotedensity)
	return
	

if __name__ == "__main__":
	osc = OSC_handler()

del midiout