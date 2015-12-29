## Evan Parton (eparto01)
## COMP 135
## December 3, 2015
## PP4: Kernel Perceptron

from math import sqrt,pow,e
import os
import os.path
import sys

#GLOBAL VARIABLES
NO = 0
YES = 1
IITERATIONS = 50

class dataSet:
	def __init__(self, filename, featuresCount=0, samples=[],
			samplesCount = 0, tau = 0, weightV=[], alphaV=[],
			type = "", numParam = 0.0):
		self.filename = filename
		self.featuresCount = 0
		self.samples = []
		self.samplesCount = 0
		self.tau = 0
		self.weightV = []
		self.alphaV = []
		self.type = type
		self.numParam = numParam
	def giveTestFilename(self):
		testFilename= self.filename.split('Train.arff')[0] + "Test.arff"
		if (not os.path.exists(testFilename)):
			print "Error finding test files in directory\n"
			quit()
		return testFilename
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
					elif '@DATA' in line:
						dataStart = 1
				##############################################
		self.samplesCount = len(self.samples)
	def setParams(self, type, numParam):
		self.type = type
		self.numParam = numParam
	def saveSample(self, line):
		newSample = dataSample(line, self.featuresCount)
		newSample.parse()
		self.samples.append(newSample)
	def initializeWeightV(self):
		del self.weightV[:]
		self.weightV = []
		if (len(self.weightV) != 0):
			print "Error with initializing weights\n"
			quit()
		self.weightV = [0]*self.featuresCount
	def initializeAlphaV(self):
		del self.alphaV[:]
		self.alphaV = []
		if (len(self.alphaV) != 0):
			print "Error with initializing Alpha weights\n"
			quit()
		self.alphaV = [0]*self.samplesCount
	def addColumn1s(self):
		for samp in range(self.samplesCount):
			lengthCheck = len(self.samples[samp].values) + 1
			self.samples[samp].values.append(1.0)
			self.samples[samp].featuresCount += 1
			lengthCheck2 = len(self.samples[samp].values)
			if (lengthCheck != lengthCheck2):
				print "Critical error adding a column of 1s"
				quit()
		self.featuresCount += 1
	def calculateTau(self):
		sum = 0.0
		for i in range(self.samplesCount):
			if (self.type == "PRIM"):
				sum += vectorSize(self.samples[i].values)
			elif ((self.type == "POLY") or (self.type=="RBF")):
				sum += sqrt(kernelFun(self.type,self.numParam,
						self.samples[i].values,
						self.samples[i].values))
			else:
				print "Error calculating tau with type"
				quit()
			self.tau = 0.1 * (sum / float(self.samplesCount))
	def updateW(self, sample):
		for k in range(self.featuresCount):
			self.weightV[k] += (sample.values[k]*sample.givenLabel)
	def runPrimalTraining(self):
		self.initializeWeightV()
		for i in range(IITERATIONS):
			for samp in range(self.samplesCount):
				dotProdResult = \
				 dotProd(self.samples[samp].values,self.weightV)
				outputO = signfun(dotProdResult) #not used
				if ((self.samples[samp].givenLabel * \
					dotProdResult) < self.tau):
					self.updateW(self.samples[samp])
	def runDualTraining(self):
		self.initializeAlphaV()
		for i in range(IITERATIONS):
			for samp in range(self.samplesCount):
				sum = 0.0
				for k in range(self.samplesCount):
					sum += \
					 ((self.alphaV[k]) * \
					  (self.samples[k].givenLabel) * \
					  (kernelFun(self.type, self.numParam,
						    self.samples[k].values,
						    self.samples[samp].values)))
				outputO = signfun(sum)		#not used
				if ((self.samples[samp].givenLabel * sum) \
				    < self.tau):
					self.alphaV[samp] += 1.0
class dataSample:
	def __init__(self,dataLine, featuresCount, values = [], givenLabel=0.0):
		self.dataLine = dataLine
		self.featuresCount = featuresCount
		self.values = []
		self.givenLabel = givenLabel
	def parse(self):
		for i in range(self.featuresCount):
			self.values.append(float(self.dataLine.split(',')[i]))
		self.givenLabel = \
			float(self.dataLine.split(',')[self.featuresCount])
def signfun(dotProdResult):
	if (dotProdResult < 0):
		return -1
	return 1
def logProb(number):
	return log(number,2)
def vectorSize(vector):
	## Need to calculate norm of vector, ||vector|| = sqrt of sum of sq's
	sum = 0.0
	for i in range(len(vector)):
		sum += pow(vector[i],2)
	return sqrt(sum)
def EucDist(list1, list2):
	distSq = 0.0
	if (len(list1) != (len(list2))):
		print "EucDist: list size not same! error! (%d,%d)" %\
			(len(list1), len(list2))
		quit()
	for i in range(len(list1)):
		distSq += float(pow(float(list1[i]) - float(list2[i]), 2))
	return float(sqrt(distSq))
def dotProd(vector1, vector2):
	if (len(vector1) != (len(vector2))):
		print "Dot Product: vector size not same! error! (%d,%d)" %\
			(len(vector1), len(vector2))
		quit()
	sum = 0.0
	for i in range(len(vector1)):
		sum += ((vector1[i] * vector2[i]))
	return sum
def kernelFun(type, numParam, vector1, vector2):
	if (len(vector1) != (len(vector2))):
		print "kernelFun: vector size not same! error! (%d,%d)" %\
			(len(vector1), len(vector2))
		quit()
	## kernelFun never called for: (type == "PRIM")
	elif (type == "POLY"):			## KERNEL FOR Polynomial
		sum = dotProd(vector1,vector2)
		sum += 1.0
		return pow(sum,numParam)
	elif (type == "RBF"):			## KERNEL FOR RBF
		s = numParam
		eExp = (-1.0 * pow(EucDist(vector1,vector2),2) / (2*pow(s,2)))
		return pow(e,eExp)
	else:
		print "Incorrect kernel function call"
		quit()
def dualPercepSum(trainFile,testSample):
	sum = 0.0
	for k in range(trainFile.samplesCount):
		sum += (trainFile.alphaV[k] * trainFile.samples[k].givenLabel *\
			kernelFun(trainFile.type,trainFile.numParam,
				   trainFile.samples[k].values,
				   testSample.values))
	return sum
def testDataset(trainFile):
	accuracy = 0.0
	testFilename = trainFile.giveTestFilename()
	testFile = dataSet(testFilename)
	testFile.readSamples()
	if (trainFile.type == "PRIM"):
		testFile.addColumn1s()
	for i in range(testFile.samplesCount):
		if (trainFile.type == "PRIM"):
			outputO = signfun(dotProd(testFile.samples[i].values,
							trainFile.weightV))
		elif ((trainFile.type == "POLY") or (trainFile.type == "RBF")):
			outputO = signfun(dualPercepSum(trainFile,
							testFile.samples[i]))
		else:
			print "Error testing with kernel type"
			quit()
		## RECORD ACCURACY by testing our prediction by the true label
		if (outputO == testFile.samples[i].givenLabel):
			accuracy += 1.0
	accuracy /= testFile.samplesCount
	return accuracy

def labelOfClosest(trainFile, sample, type, numParam):
	best = sys.maxint
	bestLabel = 0		#initialize to impossible value for sanity check
	for samp in range(trainFile.samplesCount):
		if (type == "PRIM"):
			dist = EucDist(trainFile.samples[samp].values,
					sample.values)
		elif ((type == "POLY") or (type == "RBF")):
			vectorU = trainFile.samples[samp].values
			vectorV = sample.values
			dist = \
			   sqrt(kernelFun(type,numParam,vectorU,vectorU) +
				kernelFun(type,numParam,vectorV,vectorV) -
				(2 * kernelFun(type,numParam,vectorU,vectorV)))
		else:
			print "Problem with 'types' in labeling kNN\n"
			quit()
		if (dist<best):
			best = dist
			bestLabel = trainFile.samples[samp].givenLabel
	return bestLabel
def runKNN(trainFile,type, numParam):
	accuracy = 0.0
	testFilename = trainFile.giveTestFilename()
	testFile = dataSet(testFilename)
	testFile.readSamples()
	if (type == "PRIM"):
		testFile.addColumn1s()
	for testSamp in range(testFile.samplesCount):
		label = labelOfClosest(trainFile,testFile.samples[testSamp],
					type, numParam)
		if (label == testFile.samples[testSamp].givenLabel):
			accuracy += 1.0
	accuracy /= testFile.samplesCount
	if (type == "PRIM"):
		return "%.3f"% accuracy
	return ",%.3f"% accuracy
	
def runPerceptron(trainFile,type,numParam):
	trainFile.setParams(type,numParam)
	trainFile.calculateTau()
	if (type == "PRIM"):
		trainFile.runPrimalTraining()
		return "%.3f"% testDataset(trainFile)
	trainFile.runDualTraining()
	return ",%.3f"% testDataset(trainFile)
	
def trainDataset(filename):
	knn_results = ""
	perc_results = ""

	###### PRIMAL ######
	trainFile = dataSet(filename)
	trainFile.readSamples()
	trainFile.addColumn1s()
	knn_results+= runKNN(trainFile,"PRIM", 0)
	perc_results+= runPerceptron(trainFile,"PRIM",0)

	###### DUAL (Poly / RBF) #######
	## Fresh readSamples() without added column of 1s
	trainFile = dataSet(filename)
	trainFile.readSamples()   
	for d in [1,2,3,4,5]:
		knn_results+= runKNN(trainFile,"POLY", d)
		perc_results+= runPerceptron(trainFile,"POLY",d)
	for s in [0.1, 0.5, 1.0]:
		knn_results+= runKNN(trainFile,"RBF", s)
		perc_results+= runPerceptron(trainFile,"RBF",s)
	## PRINT RESULTS ##
	print knn_results
	print perc_results
		
def printUsage():
	print "usage: %s <valid data file directory OR '*Train.arff' file>" %\
		 (sys.argv[0])
	quit()
def init():
	files = []
	argcount = 0
	if (len(sys.argv) != 2):
		printUsage()
	elif (os.path.isdir(sys.argv[1])):
		dataDir = sys.argv[1]
		if (dataDir[len(dataDir)-1] != '/'):
			dataDir += '/'
		for allfiles in os.walk(dataDir):
			for filenames in allfiles:
				for filename in filenames:
					if 'Train.arff' in filename:
						########## EACH FILE ########
						files.append(dataDir + filename)
						#############################
	elif ((os.path.exists(sys.argv[1])) and ('Train.arff' in sys.argv[1])):
		files.append(sys.argv[1])
	else:
		printUsage()
	return files
########### PROGRAM START ##########
files = init()
for file in files:
	trainDataset(file)
