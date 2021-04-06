verbose = False
import rtmidi
import random
import copy
import time
import CPPN
import numpy as np

beatid = 0
c_time = 0

class Robot:
	def __init__(self,ID):
		self.time = 0
		self.id = ID

class Libraries:
	def __init__(self,style = "beat"):
		print("library style = " , style)
		self.bassnotelib = [60,61,48,49]
		self.midinotelib = [62,63,64,65,66,67,68,69,70,71]
		off = 26
		for n in range(3):
			off +=12
			for i in range(10):
				self.midinotelib.append(i+off)
		# pentatonic
		if style == "squiggles":
			self.midinotelib = [64,65,66,67]
			self.bassnotelib = [60,61,62,63]
		if style == "breaksimple":
			self.midinotelib = [62,63,64,65,66,67,68,69,70,71]
			self.bassnotelib = [60,61]
		if style == "pentatonic":
			self.bassnotelib = [37,39,42,44,46]
			self.midinotelib = [49,56,61,63,66,68,70,73,75,78,80,82]
		elif style == "major":
			# major
			self.bassnotelib = [36,38,40,41,43,45]
			self.midinotelib = [48,55,60,62,64,65,67,69,71,72,74,76,77,79,81]
		elif style == "blues":
			self.bassnotelib = [36,43,41,39,38]
			self.midinotelib = [48,55,60,62,63,65,66,67,70,72,75,78,79]
		elif style == "minor":
			self.bassnotelib = [32,34,36,43,41,39,38]
			self.midinotelib = [48,55,60,62,63,65,67,70,72,75,79]
		self.onsets = []
		self.onsetProbs = []
		self.bassOnsetProbs = []
		for i in range(Settings.opb):
			self.onsets.append(i/Settings.opb)
		for i in range(Settings.opb):
			if (i % int(Settings.opb * 4) == 0):
				self.onsetProbs.append(Settings.probabilitymap[0])
			elif (i % int(Settings.opb * 2) == 0):
				self.onsetProbs.append(Settings.probabilitymap[1])
			elif (i % int(Settings.opb) == 0):
				self.onsetProbs.append(Settings.probabilitymap[2])
			elif (i % int(Settings.opb/2) == 0):
				self.onsetProbs.append(Settings.probabilitymap[3])
			elif (i % int(Settings.opb/4) == 0):
				self.onsetProbs.append(Settings.probabilitymap[4])
			elif (i % int(Settings.opb/8) == 0):
				self.onsetProbs.append(Settings.probabilitymap[5])
			else:
				self.onsetProbs.append(0.0)
		for i in range(Settings.opb):
			if (i % int(Settings.opb * 4) == 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[0])
			elif (i % int(Settings.opb * 2)== 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[1])
			elif (i % int(Settings.opb) == 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[2])
			elif (i % int(Settings.opb /2) == 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[3])
			elif (i % int(Settings.opb /4) == 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[4])
			elif (i % int(Settings.opb /8) == 0):
				self.bassOnsetProbs.append(Settings.bprobabilitymap[5])
			else:
				self.bassOnsetProbs.append(0.0)
			
class MidiPortHandler:
	def __init__(self, port):
		self.notesPlaying = []
		self.bnotesPlaying = []

		self.midiout = rtmidi.MidiOut()
		available_ports = self.midiout.get_ports()
		print("available ports:")
		print(available_ports)
		print("Connecting to port: " , available_ports[1])
		self.midiout.open_port(1)
			
		
class Settings:
	bpm = 120
	maxSustain = 1
	volumevalues = [127]
	bpm = 120
	# sequences per beat
	spb = 4
	opb = 8
	sequenceLength = 32
	notedensity = 1.8
	bnotedensity = 1.8
	n_seq_lines = 1
	bn_seq_lines = 1
	#probabilitymap = [1.0,0.7,0.6,0.2,0.1,0.01]
	#bprobabilitymap = [1.0,1.0,1.0,1.0,0.1,0.0]
	#probabilitymap = [1.0,1.0,1.0,0.5,0.1,0.1,0.01]
	probabilitymap = [1.0,1.0,1.0,0.5,0.3,0.1,0.05]
	bprobabilitymap = [1.0,1.0,1.0,0.5,0.3,0.1,0.05]
	def __init__(self):
		self.playSquiggles = False
		self.mutrate = 0.2
		self.volume = 127
		#volumevalues = [127,100,80,60,20]
		self.notesOn = []
		self.notesOn2 = []
		

class Sequence:
	def __init__(self, l,lib, notedensity, n_seq_lines, onsetProbs):
		# length of sequence
		self.length = l
		self.notes = []
		self.lib = lib
		self.state = 0
		self.threshold = 1-notedensity
		self.n_seq_lines = n_seq_lines
		self.onsetProbs = onsetProbs
		for n in range(n_seq_lines):
			for i in range(l):
				onset = float(i) / float(Settings.opb)
				onsetNr = i%Settings.opb
				noteprob = random.uniform(0.0,onsetProbs[onsetNr])
				if random.uniform(0.0,1) < onsetProbs[onsetNr]:
					noteprob = random.uniform(0.0,1.0)
				self.notes.append(Note(random.choice(lib), random.randint(1,Settings.maxSustain), noteprob,onset))

	def play(self, interval, n_on):
		if (self.state % interval == 0 or self.state==0):
			notenr = (int)(self.state/interval)
			if self.notes[notenr].note > 0:
				n_on.append(copy.deepcopy(self.notes[notenr]))
				# playnote(self.notes[notenr].note)
		self.state+=1
		if self.state >= self.length:
			self.state = 0

	def mutate(self,mutrate, noteprob):
		self.threshold = 1-noteprob
		for i, n in enumerate(self.notes):
			if random.uniform(0.0,1.0) < mutrate:
				onset = n.onset
				onsetNr = int((onset*Settings.opb)%Settings.opb)
				noteprob = random.uniform(0.0,self.onsetProbs[onsetNr])
				if random.uniform(0.0,1) < self.onsetProbs[onsetNr]:
					noteprob = random.uniform(0.0,1.0)
				self.notes[i] = Note(random.choice(self.lib), random.randint(2,2+Settings.maxSustain),noteprob,onset)

	def update(self, cppn = None):
		# return a beat array of 16 notes
		# notes will need a probability and an onset.
		notelist = []
		noteNr = 0
		for i,note in enumerate(self.notes):
			if cppn is not None:
				input = [i/len(self.notes)]
				output = cppn.activate(input)
				if output[0] > note.prob:
					notelist.append(note)
				if output[1] > note.prob:
					notelist.append(note)
				if output[2] > note.prob:
					notelist.append(note)
				if np.max(output) < 0.01:
					return notelist
			elif note.onset < self.state + 1 and note.onset >= self.state:
				if note.prob > self.threshold:
					notelist.append(note)
		self.state+=1
		if (int(self.state * Settings.opb) % self.length == 0):
			self.state = 0	  
		return notelist

	def display(self,ax):
		x_points = []
		y_points = []
		for i,note in enumerate(self.notes):
			if (self.threshold < note.prob):
				x_points.append(i)
				y_points.append(note.note)
		ax.scatter(x_points,y_points)

class Note:
	def __init__(self, note, sustain,noteprob,onset):
		self.note = note
		self.repeat = False
		self.sus = sustain # sustain
		self.state = 0
		self.onset = onset
		self.vel = random.choice(Settings.volumevalues)
		# probability of this note being activated
		self.prob = noteprob

def playnote(m_out,note, vel):
	note_on=[0x90,note,vel] 
	if (verbose):
		print("note_on")
	m_out.send_message(note_on)

def offnote(m_out,note):
	note_off=[0x80,note,0] 
	if (verbose):
		print("note_off")
	m_out.send_message(note_off)

class Sequencer:
	def __init__(self, lib, usecppn = False):
		self.seq = Sequence(Settings.sequenceLength,lib.midinotelib,0.4,Settings.n_seq_lines,lib.onsetProbs)
		self.bseq = Sequence(Settings.sequenceLength,lib.bassnotelib,0.8,Settings.bn_seq_lines,lib.bassOnsetProbs)
		self.cppn = None
		if usecppn:
			import CPPN
			self.cppn = CPPN.CPPN(1,3)
			self.cppn.mutate()
			self.c_p = self.cppn.getPhenotype()
		
	def update(self):
		if self.cppn is not None:
			return copy.deepcopy(self.seq.update(self.c_p)), copy.deepcopy(self.bseq.update(self.c_p))
		return copy.deepcopy(self.seq.update()), copy.deepcopy(self.bseq.update())

	def mutate(self,mutationRate,bnotedensity = Settings.bnotedensity, notedensity = Settings.notedensity):
		if self.cppn:
			print("mutating cppn")
			self.cppn.mutate()
			self.c_p = self.cppn.getPhenotype()
		self.seq.mutate(mutationRate,notedensity)
		#print(bnotedensity)
		self.bseq.mutate(mutationRate,bnotedensity)

def playnotes (m_out, n_on):
	for n in n_on:
		if (n.repeat == False and n.state == 0):
			playnote(m_out.midiout,n.note, n.vel)
		n.state+=1
		if (n.state >= n.sus):
			# switch note off
			offnote(m_out.midiout,n.note)
			# remove the note from the notesOn list
			n_on.remove(n)

	# temp
def playNotes(notes,bnotes,midihandler):
	timer = 0
	for i in range(Settings.opb):
		notes_to_be_played = []
		bnotes_to_be_played = []
		for note in notes:
			ons = note.onset % 1
			if ons <= timer:
				# print("playing " , note)
				# play this note
				notes_to_be_played.append(note)
		for note in bnotes:
			ons = note.onset % 1
			if ons <= timer:
				# print("playing " , note)
				# play this note
				bnotes_to_be_played.append(note)
		#playnotes(midihandler.midiout,notes_to_be_played)
		#playnotes(midihandler.midiout,bnotes_to_be_played)
		midihandler.notesPlaying += notes_to_be_played
		midihandler.bnotesPlaying += bnotes_to_be_played
		playnotes(midihandler,midihandler.notesPlaying)
		playnotes(midihandler,midihandler.bnotesPlaying)
		for note in notes_to_be_played:
			notes.remove(note)
		for note in bnotes_to_be_played:
			bnotes.remove(note)
		timer+=1/Settings.opb
		hz = 60/Settings.bpm/Settings.opb
		time.sleep(hz)

		

def main():
	# First assign number of midi outputs
	midiouts = []	
	midiouts.append(MidiPortHandler(1))
	# set library to "squiggles" for Dr. squiggles note lib
	Settings.maxSustain = 8
	#Settings.sequenceLength = 16
	Settings.opb = 8
	Settings.sequenceLength = 32
	Settings.notedensity = 0.1
	Settings.n_seq_lines = 1

	Settings.bnotedensity = 0.4
	lib = Libraries(style = "break")
	seq = Sequencer(lib, usecppn= False)
	Settings.bpm = 120
	count = 0
	while True:
		count+=1
		# get sequence of 1 beat
		notes,bnotes = seq.update()
		playNotes(notes,bnotes,midiouts[0])
		if count % 4 == 0:
			seq.mutate(0.1)	
