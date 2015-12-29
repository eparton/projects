## Evan Parton (eparto01)
## COMP 135
## November 5, 2015
## PP3: K-Means

import random
import copy
from math import sqrt,pow,log
import os
import os.path
import sys

#GLOBAL VARIABLES
NO = 0
YES = 1

class dataSet:
	def __init__(self, filename, updatedBestClass = NO, featuresCount=0,
			classCount=0, arffClassLabels=[],
			clusters=[], samples=[]):
		self.filename = filename
		self.updatedBestClass = updatedBestClass
		self.featuresCount = featuresCount
		self.classCount = classCount
		self.arffClassLabels=[]
		self.clusters = []
		self.samples = []
	def readSamples(self):
		dataStart = 0
		with open(self.filename) as f:
			for line in f:
				if (dataStart):
					self.saveSample(line)
				else:
				######## Skip nonData lines from .arff ########
					if 'NUMERIC' in line:
						self.featuresCount += 1
					elif '@ATTRIBUTE class' in line:
						#Number comma delimited classes
						self.classCount = \
						 line.split('{')[1].count(',')+1
						#Save actual labels
						for i in range(self.classCount):
							self.arffClassLabels.  \
						     		 append(line.  \
								 split('{')[1].\
							         split('}')[0].\
							         split(',')[i])
					elif '@DATA' in line:
						dataStart = 1
				##############################################
	def saveSample(self, line):
		newSample = dataSample(line, self.featuresCount)
		newSample.parse()
		self.samples.append(newSample)
	def changeClassSize(self, size):
		self.classCount = size
class dataClass:
	def __init__(self, id=0, mean =[]):
		self.id = id
		self.mean = []
class dataSample:
	def __init__(self, dataLine, featuresCount, values = [], givenLabel="",
			clusterMembership = 0):
		self.dataLine = dataLine
		self.featuresCount = featuresCount
		self.values = []
		self.givenLabel = givenLabel
		self.clusterMembership = 0
	def parse(self):
		for i in range(self.featuresCount):
			self.values.append(self.dataLine.split(',')[i])
		self.givenLabel = self.dataLine.split(',')[self.featuresCount]
class nmiObj:
	def __init__(self, n_ij = 0, a_i = 0, b_j = 0, N = 0):
		self.n_ij = 0
		self.a_i = 0
		self.b_j = 0
		self.N = 0
def logProb(number):
	return log(number,2)
def dist(list1, list2):
	distSq = 0
	if (len(list1) != (len(list2))):
		print "EucDist: list size not same! error! (%d,%d)" %\
			(len(list1), len(list2))
		quit()
	for i in range(0,len(list1)-1):
		distSq += pow(float(list1[i]) - float(list2[i]), 2)
	return float(sqrt(distSq))
def initializeMeans(dataFile):
	dataRange = len(dataFile.samples)
	randomPerm = random.sample(range(dataRange),dataFile.classCount)
	count = 0
	for i in randomPerm:
		initialMeanClass = dataClass()
		initialMeanClass.id = count
		for j in range(0,dataFile.featuresCount):
			meanVal = dataFile.samples[i].dataLine.split(',')[j]
			initialMeanClass.mean.append(meanVal)
		dataFile.clusters.append(initialMeanClass)
		count += 1
def findBestMeans(dataFile):
	## Basic Idea: loop through samples, calculate distance to each mean
	#  for ALL clusters (also called 'class'), and record CLOSEST cluster
	dataFile.updatedBestClass = NO
	for i in range(len(dataFile.samples)):
		bestDistance = float(sys.maxint)
		tempBestClass = -1
		for j in range(dataFile.classCount):
			tempDist = dist(dataFile.samples[i].values,
					dataFile.clusters[j].mean)
			if (tempDist < bestDistance):  # RECORD THE CLOSEST MEAN
				bestDistance = tempDist
				tempBestClass = j
		## K-MEANS CONVERGENCE CRITERIA ##
		# -Check if the newly calc'd cluster mean is NEW (ie not convrg)
		if (dataFile.samples[i].clusterMembership != tempBestClass):
			dataFile.updatedBestClass = YES
		dataFile.samples[i].clusterMembership = tempBestClass

def recalcClusterMeans(dataFile):
## Basic Idea: Recalculate means, check distance to center of cluster for ONLY
##             the members of that cluster
	## --Do an update of cluster mean FOR EACH CLASS 
	for clas in range(dataFile.classCount):

		classMembers = 0
		tempMean = [0] * dataFile.featuresCount
		for samp in range(len(dataFile.samples)):
			## --Check distance to center ONLY for members of class
			if (dataFile.samples[samp].clusterMembership == clas):
				## --Add value of EACH feature
				for feat in range(dataFile.featuresCount):
					tempMean[feat] +=                     \
						float(dataFile.samples[samp]. \
							values[feat])
					# Record matches number, for mean calc
					classMembers += 1

		## So far we've tracked sums for each feature, no take average:
		for i in range(dataFile.featuresCount):
			if (classMembers > 0):
				tempMean[i] = tempMean[i] / float(classMembers)
			else:  #No members of this class, average is zero
				tempMean[i] = 0
		## Update mean kept in dataFile object with new calculation:
		del dataFile.clusters[clas].mean[:]
		dataFile.clusters[clas].mean = copy.deepcopy(tempMean)
		del tempMean[:]

def findClusterScatter(dataFile):
	sum = 0.0
	for classID in range(dataFile.classCount):
		for sampleID in range(len(dataFile.samples)):
			if (dataFile.samples[sampleID].clusterMembership == \
			     classID):
				for feature in range(dataFile.featuresCount):
		### For each sample belonging to class 'classID', add: sq diff
					sum += \
		pow((float(dataFile.samples[sampleID].values[feature])) - \
		float(dataFile.clusters[classID].mean[feature]), 2)
	return sum
def nmiCalc(nmi, dataFile,i,j):
	n_ijSum = 0
	a_iSum = 0
	b_jSum = 0
	for samp in range(len(dataFile.samples)):
		cluMem = dataFile.samples[samp].clusterMembership
		trueLab = dataFile.samples[samp].givenLabel
		trueLab = trueLab.replace("\r\n","")
		trueLabIndex = dataFile.arffClassLabels.index(trueLab)
		if (cluMem == i):
			a_iSum += 1
			if (trueLabIndex == j):
				n_ijSum += 1
		if (trueLabIndex == j):
			b_jSum += 1
	nmi.n_ij = n_ijSum
	nmi.a_i = a_iSum
	nmi.b_j = b_jSum
	nmi.N = float(len(dataFile.samples))      # make this float for division
def findNMI(dataFile):
	nmi = nmiObj()
	sumI_U_V = 0.0
	sumH_U = 0.0
	sumH_V = 0.0
	for i in range(dataFile.classCount):
		for j in range(len(dataFile.arffClassLabels)):
			nmiCalc(nmi, dataFile, i, j)
			if (nmi.n_ij == 0):
				mutInfo = 0.0
			else:
				mutInfo=((nmi.n_ij / float(nmi.N)) *           \
					   logProb((nmi.n_ij / float(nmi.N)) / \
			         float(nmi.a_i * nmi.b_j / pow(nmi.N, 2))))
			sumI_U_V += mutInfo
	###j influences only n_ij and b_j, does NOT influence a_i (so let j = 0)
	for i in range(dataFile.classCount):
		nmiCalc(nmi, dataFile, i, 0)
		if (nmi.a_i == 0):
			calc = 0.0
		else:
			calc=((nmi.a_i/nmi.N) * logProb(nmi.a_i / nmi.N))
		sumH_U += calc
	sumH_U *= (-1)
	###i influences only n_ij and a_i, does NOT influence b_j (so let i = 0)
	for j in range(len(dataFile.arffClassLabels)):
		nmiCalc(nmi, dataFile, 0, j)
		if (nmi.b_j == 0):
			calc = 0.0
		else:
			calc=((nmi.b_j/nmi.N) * logProb(nmi.b_j / nmi.N))
		sumH_V += calc
	sumH_V *= (-1)
	return ((2 * sumI_U_V) / (sumH_U + sumH_V))
def runKMeans(dataFile):
	dataFile.updatedBestClass = YES
	initializeMeans(dataFile)
	findBestMeans(dataFile)
	while (dataFile.updatedBestClass == YES):
		recalcClusterMeans(dataFile)
		findBestMeans(dataFile)
def runDataSet(filename):
	dataFile = dataSet(filename)
	dataFile.readSamples()
	runKMeans(dataFile)
	CS = findClusterScatter(dataFile)
	NMI = findNMI(dataFile)
	######### PRINTING RESULTS ########
	print "Cluster Scatter: %f" % CS
	print "NMI: %f" % NMI
def problem3_2(filename):
	print "Processing: %s (10 runs with random initialization)" % (filename)
	for i in range(10):
		runDataSet(filename)
def problem3_3(filename):
	print "Processing: %s (with 15 values for k)" % (filename)
	dataFile = dataSet(filename)
	dataFile.readSamples()
	for i in range(15):
		dataFile.changeClassSize(i+1)
		runKMeans(dataFile)
		CS = findClusterScatter(dataFile)
		######### PRINTING RESULTS ########
		print "(k = %d) Cluster scatter: %f" % (i+1, CS)
def printUsage():
	print "usage: %s <valid data file directory>" % (sys.argv[0])
	quit()
def init():
	files = []
	argcount = 0
	if (len(sys.argv) != 2):
		printUsage()
	else:
		dataDir = sys.argv[1]
		if (dataDir[len(dataDir)-1] != '/'):
			dataDir += '/'
		if (os.path.isdir(dataDir) != True):
			printUsage()
	for allfiles in os.walk(dataDir):
		for filenames in allfiles:
			for filename in filenames:
				if '.arff' in filename:
					########## EACH FILE ########
					files.append(dataDir + filename)
					#############################
	return files
########### PROGRAM START ##########
random.seed(1001)
files = init()
for file in files:
	problem3_2(file)
for file in files:
	problem3_3(file)
