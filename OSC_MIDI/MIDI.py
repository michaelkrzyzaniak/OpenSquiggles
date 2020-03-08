import time
import rtmidi
import random
import copy
import argparse
from pythonosc import dispatcher as DPatcher
from pythonosc import osc_server
import numpy as np

from pythonosc import udp_client
import math
import Sequencer as seq
from enum import Enum

import pygame

midiout = rtmidi.MidiOut()
midiout2 = rtmidi.MidiOut()
available_ports = midiout.get_ports()

# TODO: add sequencer to robot class so that individual squiggles can get sequencers

class MusicType:
	MIDI = 0,
	OSC = 1,
	OSC_WITH_INPUT = 2
	

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
		self.ID = ID
		self.lib = seq.Libraries("squiggles")
		self.seq = seq.Sequencer(self.lib)
		self.seq.mutate(0.1)

# temp
# squiggleslib = [62,63,64,65,66,67,59,68,69,70,71]
squiggleslib = [62,63,64,65,66,67]
# squiggleslib = [59]
# squiggleslib = [38,39,40,41,42,43,44,45,46,47,50,51,52,53,54,55,56,57,58,59,62,63,64,65,66,67,59,68,69,70,71]
# squiggleslib = [60]
squigglesbasslib = [60,61]

class OSC_handler:
	def __init__(self):
		r_times = []
		# note lookup
		# self.lib = seq.Libraries("squiggles")
		# sequencer to use # note, no support for multiple sequencers yet
		#self.seq = seq.Sequencer(self.lib)
		#self.seq.mutate(1.0)

		self.robots = []

		self.mutation_probability = 0.04

		self.bpm_activation_level = 0.0
		self.leakiness = 0.99
		self.bpm = 50
		self.bpm_max = 200
		self.bpm_min = 50

		self.density_r = 0.0
		self.density_l = 0.0
		


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

		self.counter = 0.0
		dispatcher = DPatcher.Dispatcher()
		#dispatcher.map("/beat", beat_handler, "beat")
		dispatcher.map("/beat", self.broadcast, "rhythm")
		dispatcher.map("/bpm", self.setbpm, "bpm")
		dispatcher.map("/emg", self.broadcastemg, "emg")

		self.emg_signal_l = [] # array of 8 values
		self.emg_signal_r =[]
		self.emg_right_buffer = []
		self.emg_left_buffer = []

		self.acc_l = 0.0
		self.acc_r = 0.0
		self.acc_left_buffer = []
		self.acc_right_buffer = []
		
		self.acc_l_max = 0.0
		self.acc_r_max = 0.0
		
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

	def setbpm(self,unused_addr, args,bpm):
		print("setting bpm to ", bpm)
		seq.Settings.bpm = bpm
	def transpose(self,l__) :
		m_array = []
		if len(l__) < 1:
			return m_array
		for j in range(len(l__[0])):
			array = []
			for i in range(len(l__)):
				array.append(l__[i][j])
			m_array.append(array)
		return m_array

	def broadcast(self,unused_addr, args,timestamp, robot_id, beat_id, use_emg = False):
		if use_emg == False:
			seq.Settings.probabilitymap =	[1.0,1.0,1.0,0.5,0.2,0.1,0.0]
			seq.Settings.bprobabilitymap =	[1.0,1.0,1.0,0.5,0.2,0.1,0.0]
			self.counter+=0.05
			
			#print("prob",prob)
			#seq.Settings.probabilitymap[5] = prob
			#seq.Settings.probabilitymap[6] = prob
			#seq.Settings.bprobabilitymap[5] = prob
			#seq.Settings.bprobabilitymap[6] = prob
			#seq.Settings.probabilitymap[3] = prob
			#seq.Settings.probabilitymap[4] = prob
			#seq.Settings.bprobabilitymap[3] = prob
			#seq.Settings.bprobabilitymap[4] = prob

		#print("self.emg_signal_r:", len(self.emg_signal_r))
		#print("self.emg_signal_l:", len(self.emg_signal_l))
		# Get new sequence snippet
		#print("message received: ", args)
		if(len(self.emg_left_buffer) > 200):
			length = len(self.emg_left_buffer)
			for i in range(length-200):
				self.emg_left_buffer.pop()
		if(len(self.emg_right_buffer) > 200):
			length = len(self.emg_right_buffer)
			for i in range(length-200):
				self.emg_right_buffer.pop()


		robot_present = False
		c_robot = None
		flush = True;
		for i,r in enumerate(self.robots):
			if r.ID == robot_id:
				robot_present = True
				c_robot = r
				if i == 0:
					flush = True
		if robot_present == False:
			c_robot = Robot(robot_id)
			self.robots.append(c_robot)


		notes,bnotes = c_robot.seq.update()
		notes = notes+bnotes
		if use_emg:
			if (len(self.emg_left_buffer) < 2):
				return
			if (len(self.emg_right_buffer) < 2):
				return
	
			l_avg = []
			l_1tot = []
			l_2tot = []
			l_1max = []
			l_2max = []
			l__ = []
			for vals in self.emg_left_buffer:
				l_ = []
				for i,value in enumerate(vals):
					l_.append(value)
					#l1_.append(value)
				l_avg.append(np.max(vals))
				l__.append(l_)


			l__ = self.transpose(l__)

			#l__ = np.array(l__)
			#print(l__)

			#l__.transpose()
			#print(l__)

			r__ = []
			for vals in self.emg_right_buffer:
				r_ = []
				for i,value in enumerate(vals):
					r_.append(value)
					#l1_.append(value)
				r__.append(r_)
			
			r__ = self.transpose(r__)
			maxval = 0.995
			if robot_id == 0: #( c_robot is self.robots[0]):
				c_robot.seq.mutate(0.01,self.density_l,self.density_l)
				#print("len buf left" ,len(self.emg_right_buffer));
				if (flush):
					if (len(self.acc_left_buffer) >  0):
						self.acc_l_max = np.max(self.acc_left_buffer)
					self.emg_left_buffer = []
					self.emg_signal_l = [] # array of 8 values
					self.acc_left_buffer = []
		
					#if len(self.robots == 1):
					#self.emg_right_buffer = []
					#self.emg_signal_r =[]
			else: 
				#elif (len(self.robots) > 1 and c_robot is self.robots[1]):
				c_robot.seq.mutate(0.01,self.density_r,self.density_r)
				#print("len buf right" ,len(self.emg_right_buffer));
				if (flush):
					if (len(self.acc_right_buffer) >  0):
						self.acc_r_max = np.max(self.acc_right_buffer)
					self.emg_right_buffer = []
					self.emg_signal_r =[]
					self.acc_right_buffer = []

		else:
			#self.mutation_probability = math.sin(self.counter)/10
			c_robot.seq.mutate(self.mutation_probability,bnotedensity = 0.8,notedensity = 0.8)

		messageString = []
		maxer = 0.0
		max_density = 0.97
		leakiness_density = 0.95
		offset_density = 0.6
		if use_emg:
			for vals in l__:
				maxer+=np.max(vals) /16.0
			self.density_l += maxer 
			print("max: " , self.density_l)
			self.density_l *= leakiness_density 
			self.density_l = offset_density + self.acc_l_max
			if self.density_l > max_density:
				self.density_l = max_density

			for vals in r__:
				maxer+=np.max(vals) /16.0
			self.density_r += maxer 
			print("max: " , self.density_r)
			self.density_r *= leakiness_density
			self.density_r = offset_density + self.acc_r_max 
			if self.density_r > max_density:
				self.density_r = max_density
		else:
			self.density_l = 1.0
			self.density_r = 1.0


		for note in notes:
			intensity = 0.0
			#print("note.note-60 : ", note.note-60)
			if use_emg:
				if robot_id == 1: #(c_robot is self.robots[0]):
					#print(l__[note.note-60])
					if len(l__) > 0:
						intensity = np.max(l__[note.note-60])
					else:
						intensity = 0.1
				else:
					if len(r__) > 0:
						intensity = np.max(r__[note.note-60])
					else:
						intensity = 0.1
			
				#if (intensity > 0.1):
			#	messageString.append(1.0)
			#else:
			if not use_emg:
				intensity = 0.5
				#prob = math.sin(self.counter) 
				#print("prob",prob)
				#if prob < 0.0:
				#	prob = 0.0
				#elif prob > 0.5:
				#	prob = 1.0
				#intensity = prob
			if intensity > 0.1 or not use_emg:
				while note.onset >= 1.0:
					note.onset = note.onset - math.floor(note.onset)
				#if (note.note ==62or note.note == 66):
				messageString.append(note.onset)
				messageString.append(note.note-60)
				
				if intensity > 0.2:
					messageString.append(intensity)
				else:
					messageString.append(1)
			
			
		msg = [robot_id,beat_id] + messageString #0,-1,-1,0.11,-1,-1,0.22,-1,-1,0.33,-1,-1,0.5,-1,-1,0.72,-1,-1,0.88,-1,-1]
		print(msg)
		self.client.send_message("/rhythm", msg)
		self.bpm = self.bpm_min+ (self.bpm_activation_level *4)
		#print("bpm: ",self.bpm)
		if self.bpm > self.bpm_max:
			self.bpm = self.bpm_max
		#if c_robot.ID == self.robots[0].ID:
		#	self.client.send_message("/tempo", float(self.bpm))
		if len(self.emg_left_buffer)>2:
			self.emg_left_buffer = []
			self.emg_signal_l = [] # array of 8 values
		if len(self.emg_right_buffer)>2:
			#if len(self.robots == 1):
			self.emg_right_buffer = []
			self.emg_signal_r =[]

		return 
		# old


	def broadcast3(self,unused_addr, args,timestamp, robot_id, beat_id, use_emg = True):
		#print("self.emg_signal_r:", len(self.emg_signal_r))
		#print("self.emg_signal_l:", len(self.emg_signal_l))
		# Get new sequence snippet
		#print("message received: ", args)
		robot_present = False
		c_robot = None
		flush = True;
		for i,r in enumerate(self.robots):
			if r.ID == robot_id:
				robot_present = True
				c_robot = r
				if i == 0:
					flush = True
		if robot_present == False:
			c_robot = Robot(robot_id)
			self.robots.append(c_robot)


		notes,bnotes = c_robot.seq.update()
		notes = notes+bnotes
		if use_emg:
			if (len(self.emg_left_buffer) < 1):
				return
			if (len(self.emg_right_buffer) < 1):
				return
			l_avg = []
			l_1tot = []
			l_2tot = []
			l_1max = []
			l_2max = []
			for vals in self.emg_left_buffer:
				l_1 = []
				l_2 = []
				for i,value in enumerate(vals):
					if (i >1 and i < 6):
						#print("ADD",i, end = "")
						l_1.append(value)
					else:
						l_2.append(value)
				l_avg.append(np.max(vals))
				#print(l_1)
				#print(l_2)
				l_1max.append(np.max(l_1))
				l_2max.append(np.max(l_2))

			r_avg = []
			r_1tot = []
			r_2tot = []
			r_1max = []
			r_2max = []
			for vals in self.emg_right_buffer:
				r_1 = []
				r_2 = []
				for i,value in enumerate(vals):
					if (i >1 and i < 6):
						#print("ADD",i, end = "")
						r_1.append(value)
					else:
						r_2.append(value)
				r_avg.append(np.max(vals))
				#print(l_1)
				#print(l_2)
				r_1max.append(np.max(r_1))
				r_2max.append(np.max(r_2))

			# 3456
			# 1278
			# mutate the sequence with a mutation rate
			# r_avg =  np.max(self.emg_right_buffer) * 2
			
			r_avg_s =  np.average(r_1max)
			# increase BPM activation level:
			self.bpm_activation_level+=r_avg_s
			self.bpm_activation_level*=self.leakiness 
			#self.bpm_activation_level +=
			#print(l_1max)
			l__1 = np.max(l_1max)*1 + 0.2
			l__2 = np.max(l_2max)*1 + 0.2
			r__1 = np.max(r_1max)*2+ 0.2
			r__2 = np.max(r_2max)*2 + 0.2
			maxval = 0.995
			if l__1 > maxval:
				l__1 = maxval
			if l__2 > maxval:
				l__2 = maxval
			if r__1 > maxval:
				r__1 = maxval
			if r__2 > maxval:
				r__2 = maxval
			#print(l__1,l__2,r_avg_s)
			if (c_robot is self.robots[0]):
				#c_robot.seq.mutate(0.01,bnotedensity = l__1,notedensity = l__2)
				#print("len buf left" ,len(self.emg_right_buffer));
				if (flush):
					self.emg_left_buffer = []
					self.emg_signal_l = [] # array of 8 values
		
			elif (len(self.robots) > 1 and c_robot is self.robots[1]):
				#c_robot.seq.mutate(0.01,bnotedensity = r__1,notedensity = r__2)
				#print("len buf right" ,len(self.emg_right_buffer));
				if (flush):
					self.emg_right_buffer = []
					self.emg_signal_r =[]

		else:
			c_robot.seq.mutate(0.1,bnotedensity = 0.8,notedensity = 0.8)
		messageString = []
		for note in notes:
			if note.onset >= 1.0:
			#	print("onset",note.onset)
				note.onset = note.onset - math.floor(note.onset)
			#print(note.note)
			#if note.note != 61:
			#	continue
				#continue
			messageString.append(note.onset)
			messageString.append(note.note-60)
			if r__2 < 0.7:
				messageString.append(0.0)
			#	print("intensity 0")
			else:
			#	print("intensity 1")
				messageString.append(1)

		msg = [robot_id,beat_id] + messageString #0,-1,-1,0.11,-1,-1,0.22,-1,-1,0.33,-1,-1,0.5,-1,-1,0.72,-1,-1,0.88,-1,-1]
		#print(msg)
		self.client.send_message("/rhythm", msg)
		self.bpm = self.bpm_min+ (self.bpm_activation_level *4)
		#print("bpm: ",self.bpm)
		if self.bpm > self.bpm_max:
			self.bpm = self.bpm_max
		#if c_robot.ID == self.robots[0].ID:
		#	self.client.send_message("/tempo", float(self.bpm))
		return 
		# old
	def broadcast2(self,unused_addr, args,timestamp, robot_id, beat_id, use_emg = True):
		if (self.emg_signal_r > 0.02):
			print("sending something insane to squiggles")
			messageString = []
			timer = 0.0
			messageString.append(timer)
			messageString.append(7)
			messageString.append(-1)
			messageString.append(0.5)
			messageString.append(random.randint(0,7))
			messageString.append(-1)
			messageString.append(0.66)
			messageString.append(random.randint(0,7))
			messageString.append(-1)
			messageString.append(0.75)
			messageString.append(random.randint(0,7))
			messageString.append(-1)
			#midiID = int(10*self.emg_signal_r)
			for i,val in enumerate(self.emg_signal_l):
				if val > 0.02:
					timer+=0.1
					messageString.append(timer)
					messageString.append(7-i)
					messageString.append(val)
		
			#for i in range(int(self.emg_signal_l*9)):
			#	timer+=0.3
			#	messageString.append(timer)
			#	messageString.append(midiID)
			#	messageString.append(midiID)

			msg = [robot_id,beat_id] + messageString #0,-1,-1,0.11,-1,-1,0.22,-1,-1,0.33,-1,-1,0.5,-1,-1,0.72,-1,-1,0.88,-1,-1]
			print(msg)
			self.client.send_message("/rhythm", msg)
		#return
		#elif (self.emg_signal > 0.7):
		#	print("sending something crazy to squiggles")
		#	msg = [robot_id,beat_id,0,-1,-1,0.11,-1,-1,0.22,-1,-1,0.33,-1,-1,0.5,-1,-1,0.72,-1,-1,0.88,-1,-1]
		#	self.client.send_message("/rhythm", msg)
		#elif (self.emg_signal > 0.3):
		#	print("sending something to squiggles")
		#	msg = [robot_id,beat_id,0,-1,-1,0.33,-1,-1,0.66,-1,-1]
		#	self.client.send_message("/rhythm", msg)
		#elif (self.emg_signal > 0.2):
		#	print("sending something to squiggles")
		#	msg = [robot_id,beat_id,0,-1,-1]#,0.33,-1,-1,0.66,-1,-1]
		#	self.client.send_message("/rhythm", msg)
		
	def broadcastemg(self,unused_addr, args, *a):
		#print("received emg:", a)
		limit = 1
		acc_val = 16
		if len(a) > 10:
			limit = 8
		if len(a) >0:
			self.emg_signal_l = []
			self.emg_signal_r = []
			for i,val in enumerate(a):
				if i < limit :
					self.emg_signal_r.append(val)
				elif i >= acc_val:
					self.acc_l = val
					self.acc_r = val
				else:
					self.emg_signal_l.append(val)
			self.emg_left_buffer.append(self.emg_signal_l)
			self.emg_right_buffer.append(self.emg_signal_r)
			self.acc_left_buffer.append(self.acc_l)
			self.acc_right_buffer.append(self.acc_r)


	def update(self):
		# receive message TODO

		# send message
		beatid+=1
		self.client.send_message("/rhythm", self.robots[0].id, beatid, 100.0,20)


#if available_ports:
#	midiout.open_port(1)
	#midiout.open_port(2)
	#midiout2.open_port(1)
#else:
#	midiout.open_virtual_port("Virtual Port")

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


if __name__ == "__main__":
	type = MusicType.OSC_WITH_INPUT
	if (type == MusicType.MIDI):
		# Starts a sequencer
		seq.main()
		print("...")
	elif (type == MusicType.OSC_WITH_INPUT):
		osc = OSC_handler()



del midiout