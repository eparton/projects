## Evan Parton (eparto01)
## COMP 135
## Oct 13, 2015
## PP2 -- Bayes text classification

import random
import copy
import math

#GLOBAL VARIABLES
NO = 0
YES = 1
TRAIN = 0
TEST = 1
CROSSVALFOLDNUM = 10

def logProb(number):
	if (number <= 0):
		return -1000000
	return math.log(number,2)
class SampleDoc:
	def __init__(self,index=0, traintestType=-1,
			label=0, words=[], wordFrequency=[], wordCount=0):
		self.index = index
		self.label = label
		self.traintestType = traintestType
		self.words = words
		self.wordFrequncy = wordFrequency
		self.wordCount = wordCount
## Single class object that holds database of words and vocab
## Holds SampleDoc object for each of the ingested files
## --> revision notes (see write up): move vocab into separate class
class wordDatabase:
	def __init__(self, classType, dataDir,
			smoothingM=0.0, validSampleNumbers=[],
			sampleLabels=[], totalNumSamples=0, sampleDocs=[],
			wordCountNO=0, wordCountYES=0, vocab=[],
			vocabCountNO = 0, vocabCountYES = 0,
			vocabFreqNO = [], vocabFreqYES = []):
		self.classType = classType
		self.dataDir = dataDir
		self.smoothingM = smoothingM
		self.indexFull = dataDir + classType + "/index.Full"
		self.indexShort = dataDir + classType + "/index.Short"
		self.validSampleNumbers = validSampleNumbers
		self.sampleLabels = sampleLabels
		self.totalNumSamples = totalNumSamples
		self.sampleDocs=sampleDocs
		self.wordCountNO = wordCountNO
		self.wordCountYES = wordCountYES
		self.vocab = vocab
		self.vocabCountNO = vocabCountNO
		self.vocabCountYES = vocabCountYES
		self.vocabFreqNO = vocabFreqNO
		self.vocabFreqYES = vocabFreqYES
	def initSamples(self, smoothingM):
		self.smoothingM = smoothingM
		self.readLabels()
		self.ingestSamples()
		sum = 0
		for i in range(0,self.totalNumSamples):
			sum += self.sampleDocs[i].wordCount

	def sampleNumToIndex(self, sampleNum):
			# PERFORMANCE CONSIDERATION (with .index)- minimize call
		return self.validSampleNumbers.index(sampleNum)
	def indexToSampleNum(self, index):
		return self.validSampleNumbers[index]
	def readLabels(self):
		del self.validSampleNumbers[:]
		self.validSampleNumbers = []
		################ SHORT OR FULL?? ###############
		#with open(self.indexShort) as f:
		with open(self.indexFull) as f:
			for line in f:
				self.validSampleNumbers.\
					append(int(line.split('|')[0]))
				self.sampleLabels.append(line.split('|')[1])
		self.totalNumSamples = len(self.validSampleNumbers)
	def ingestSamples(self):
		del self.sampleDocs[:]
		self.sampleDocs = []
		words = []
		sampleBuilder = SampleDoc()
		for i in range(0,self.totalNumSamples):
			del sampleBuilder.words[:]
			del words[:]
			file = self.dataDir + self.classType + \
				"/" + str(self.indexToSampleNum(i)) + ".clean"
			with open(file) as f:
				for line in f:
					for atom in line.split():
						words.append(atom)
			label = \
			   self.sampleLabels[i]
			sampleBuilder.index = i
			sampleBuilder.label = label
			sampleBuilder.words=words
		## SMOOTHING -- a count for both YES / NO classes **
			if (label == 'no'):
				self.wordCountNO += len(words)
			else:  ## label == 'yes'
				self.wordCountYES += len(words)
			sampleBuilder.wordCount = len(words)
			self.sampleDocs.append(copy.deepcopy(sampleBuilder))
	def allocateTrainTestType(self,iSliceIndex, samplePerc):
		iSliceNum = iSliceIndex + 1
		sliceSize = self.totalNumSamples / CROSSVALFOLDNUM
		randomPerm = random.sample(range(self.totalNumSamples),
					    self.totalNumSamples)
		#####################################################
		### FIRST TRAIN (before slice i):
		for i in range(1, iSliceNum):
			for j in range(0,sliceSize):
				sampleNum = (i * sliceSize) + j
				self.sampleDocs[randomPerm[sampleNum]].\
					traintestType = TRAIN
		### THEN TEST (for actual slice i):
		for k in range(0,sliceSize):
			sampleNum = ((iSliceNum-1) * sliceSize) + k
			self.sampleDocs[randomPerm[sampleNum]].traintestType =\
				 TEST
		### FINALLY TRAIN (after slice i):
		for i in range(iSliceNum + 1, CROSSVALFOLDNUM):
			for j in range(0,sliceSize):
				sampleNum = ((i-1) * (sliceSize)) + j
				self.sampleDocs[randomPerm[sampleNum]].\
					traintestType = TRAIN
		##GET REMAINDER (division into cross-val groups not nec. clean)
		while (sampleNum + 1 < self.totalNumSamples):
			sampleNum += 1
			self.sampleDocs[randomPerm[sampleNum]].traintestType =\
				 TRAIN
		#####################################################
		## FOR learning curve, exclude (1-samplePerc) of group
		for i in range(0, CROSSVALFOLDNUM):
			for j in range(int(sliceSize*samplePerc), sliceSize):
				self.sampleDocs[randomPerm[j]].traintestType=-1
		#####################################################
	def clearVocab(self):	
		del self.vocab[:]
		self.vocab = []
		del self.vocabFreqNO[:]
		self.vocabFreqNO = []
		del self.vocabFreqYES[:]
		self.vocabFreqYES = []
		self.vocabCountNO = 0
		self.vocabCountYES = 0
	## --> revision notes: Keep vocab count (here: vocabFreqNO/YES) in
	##     class of sampleDocs (so to not have to recount each reslice),
	##     instead of with self (word database)
	def makeVocab(self):
		self.clearVocab()
		for i in range(0,self.totalNumSamples):
			thisDocType = self.sampleDocs[i].traintestType
			if ((thisDocType == TEST) or (thisDocType == -1)):
				continue #prevents TEST from building into vocab
			for j in range(0,self.sampleDocs[i].wordCount):
				word = self.sampleDocs[i].words[j]
				#########################################
				## Each vocab word can have a COUNT
				## Regardless of count (for > 1), keep a label
				## associated with EACH member being counted
				#########################################
				#Search for word once: return index,or -1 if not
				wordIndex = \
					next((i for i,
						x in enumerate(self.vocab) \
						if x == word), -1)
				if (wordIndex == -1): # NEW WORD, not in vocab
					self.vocab.append(word)

					## Add FreqNO to sampleDoc insteadv self
					if (self.sampleDocs[i].label == 'no'):
						self.vocabFreqNO.append(1)
						self.vocabFreqYES.append(0)
						self.vocabCountNO += 1
					else:                 #label = 'yes'
						self.vocabFreqNO.append(0)
						self.vocabFreqYES.append(1)
						self.vocabCountYES += 1
				else:
					## Else word already in vocab
					##  - increase frequency, track label
					wordIndex = self.vocab.index(word)
					if (self.sampleDocs[i].label == "no"):
						self.vocabFreqNO[wordIndex] += 1
					else:                 #label = "yes"
						self.vocabFreqYES[wordIndex] +=1
	def totalYES(self):
		sum = 0
		for i in range(0,len(self.sampleDocs)):
			if (self.sampleDocs[i].label == 'yes'):
				sum += 1
		return sum
	def totalNO(self):
		sum = 0
		for i in range(0,len(self.sampleDocs)):
			if (self.sampleDocs[i].label == 'no'):
				sum += 1
		return sum
	def runTestSet(self): #, smoothingM):
		match = 0
		badmatch = 0
		for i in range(0,self.totalNumSamples):
			sumProbsNO = 0.0
			sumProbsYES = 0.0
			## include probability of class (# in class/total docs)
			##   -> PRIOR PROBABILITY
			sumProbsNO += \
				logProb(self.totalNO()/len(self.sampleDocs))
			sumProbsYES += \
				logProb(self.totalYES()/len(self.sampleDocs))
			thisDocType = self.sampleDocs[i].traintestType
			if ((thisDocType == TRAIN) or (thisDocType == -1)):
				continue
			#+++++++++++  PROCESS EACH SINGLE TEST EMAIL  +++++++++#
			for j in range(0,self.sampleDocs[i].wordCount):
				word = self.sampleDocs[i].words[j]
				wordIndex = next((i for i,
						    x in enumerate(self.vocab) \
						      if x == word), -1)
				## New test word (-1 index), does NOT get calc'd
				if (wordIndex >= 0):

			## BAYES CALCULATION
			##   - for new: test sample d given class c_j
			##	where d is a new test sample (email) and
			##	c_j is one of the two classes NO / YES
			##   - calculation done twice: for BOTH NO / YES classes
			## p(c_j | d) = [p(d | c_j) * p(c_j)] / p(d)
			## p(c_j | d) = prob. of a class given a new sample
			##	    => prob an email is class NO / YES
			## p(d | c_j) = prob. of the email happening given class
			##	    => calculated as PRODUCT (for each word) of
			##		  (# count of word IN class)  
			##		------------------------------
			##		(# of word locations IN class)
			## p(c_j)     = prob. of class happening (around 0.5
			##		 since sim. number of NO / YES emails)
			## p(d)       = prob. of an email (excluded: a constant
			##		 in both classes; divided out to cancel)
				############# LePLACE SMOOTHING ############
				##
					## FOR NEW WORDS from test docs
					## --> Revision notes- make modular:
					##    -There is repetition of code here
					## 1 -- not in NO vocab:
					if (self.vocabFreqNO[wordIndex] == 0):
						#print "Not in FreqNO!"
						sumProbsNO += -1000000
						sumProbsYES += logProb(\
	(float(self.vocabFreqYES[wordIndex]) + self.smoothingM) /\
	  (float(self.wordCountYES + (self.smoothingM * len(self.vocab)))))

					## 2 -- not in YES vocab:
					elif (self.vocabFreqYES[wordIndex] ==0):
						#print "Not in FreqYES!"
						sumProbsNO += logProb(\
	(float(self.vocabFreqNO[wordIndex]) + self.smoothingM) /\
	  (float(self.wordCountNO + (self.smoothingM * len(self.vocab)))))
						sumProbsYES += -1000000

					## 3 -- in both vocabs:
					else:
						sumProbsNO += logProb(\
	(float(self.vocabFreqNO[wordIndex]) + self.smoothingM) /\
	  (float(self.wordCountNO + (self.smoothingM * len(self.vocab)))))
						sumProbsYES += logProb(\
	(float(self.vocabFreqYES[wordIndex]) + self.smoothingM) /\
	  (float(self.wordCountYES + (self.smoothingM * len(self.vocab)))))
#initial attmpt					sumProbsYES += \
#	logProb(self.vocabFreqYES[wordIndex] / float(self.vocabCountYES))

						################################
				else:
					## NO influence, didn't show up in TRAIN
					1
 		
			#---------------- end process test email -------------#
			result = ""
			if ((sumProbsNO > sumProbsYES) and \
			    (self.sampleDocs[i].label == 'no')):
				match += 1
			elif ((sumProbsNO < sumProbsYES) and \
			    (self.sampleDocs[i].label == 'yes')):
				match += 1
			else:
				badmatch += 1
		if (match + badmatch == 0):
			return 0
		return float(match / float(match + badmatch))
#---------------------------------#
########### RUN PROGRAM ###########
#---------------------------------#
def splitRunSplits(f, crossValRuns, subSampPerc): #, smoothingValueM):
	sum = 0
	result = 0
	stDevSum = 0
	results = []
	#separate indicis into train & test
	for i in range(0, crossValRuns):
		f.allocateTrainTestType(i, subSampPerc)
		f.makeVocab()
		result = f.runTestSet()
		results.append(result)
		sum += result
	average = (sum / float(crossValRuns))
	for i in range(0, crossValRuns):
		stDevSum += math.pow((results[i] - average),2)
		#stDevSum += math.sqrt(diffSquared)
	stDev = math.sqrt(stDevSum)
	return [average,stDev]
	#print "%d) %f"%(i,result)
def runCategory(category, splitsNum, smoothingValueM, subSampPerc):
	##################
	## CHANGE FOR DATA DIRECTORY
	##################
	dir = "data/"

	f = wordDatabase(category, dir)
	f.initSamples(smoothingValueM)
	results = splitRunSplits(f, splitsNum, subSampPerc) #, smoothingValueM)
	print "Category (m = %.1f, subsample: %.1f): %s, average:%f, stDev:%f"%\
		(smoothingValueM,subSampPerc,category, results[0], results[1])
def runMain(splitsNum, Ms, samplePercs):
	for smoothingValueM in Ms:
		for sampleSize in samplePercs:
			runCategory("ibmmac", splitsNum, 
				     smoothingValueM, sampleSize)
			runCategory("sport", splitsNum, 
				     smoothingValueM, sampleSize)
### Start Program
# Question 4a)
Ms4a = [0.0, 1.0]
samplePercs4a = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
runMain(CROSSVALFOLDNUM, Ms4a, samplePercs4a)
# Question 4b)
Ms4b = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
      1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
samplePercs4b = [0.5]
runMain(CROSSVALFOLDNUM, Ms4b, samplePercs4b)
