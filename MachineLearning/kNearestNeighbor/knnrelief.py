from math import sqrt, fabs, pow
import sys
import os
import os.path
import subprocess
import random

#import environment variable CLASSPATH:
os.environ['CLASSPATH'] = "/roba/tufts/comp135/weka.jar"
os.environ['WEKADATA'] = "/roba/tufts/comp135/data/"

#Global Variables
#trainGlobal2d a 2-dimensional list: [dataLineNumber][featureNum] = data
trainGlobal2d=[]              
testGlobal2d = []
trainFile = ""
testFile = ""
userGivenK = 0
featuresTrainingNum = 0
kBest = []
top14 = []
top14Index = []
TRAINMODE = 0
TESTMODE = 1
OFFRELIEF = 0
ONRELIEF = 1
FEATALL = 0
FEAT14 = 1
HIT = 0
MISS = 0
reliefWeightGlo = []

def runWeka():
	processedTrain = 0
	cmdJ48=['java','weka.classifiers.trees.J48',
	                 '-t',trainFile,
	                 '-T',testFile]
	p=subprocess.Popen(cmdJ48,stdout=subprocess.PIPE,
		  stderr=subprocess.PIPE,stdin=subprocess.PIPE)
	while(True):
		retcode = p.poll()
		line = p.stdout.readline()
		## PRINTING - first prints training, then test (per prompt)
		if ('Correctly' in line):
			words = line.split()
			print "%s (%s)" % (words[3],words[4])
		if (retcode is not None):
			break
def lineToList(line):
	return line.split(",")

def EucDist(list1, list2, FRESTRICT):
	distSq = 0
	if (len(list1) != (len(list2))):
		print "EucDist: list size not same! error!"
		quit()
	else:
		#go to len(list1)-1 to EXCLUDE the classfication {0,1}
		if (FRESTRICT == FEAT14):
			for i in range(0, 14):
				distSq += pow(list1[top14Index[i]] - \
					      list2[top14Index[i]], 2)
		elif (FRESTRICT == FEATALL):
			for i in range(0,len(list1)-1):
				distSq += pow(list1[i] - list2[i], 2)
		return sqrt(distSq)
def WeightedReliefDist(list1, list2, FRESTRICT):
	distSq = 0
	if (len(list1) != (len(list2))):
		print "list size not same! error!"
		quit()
	else:
		#go to len(list1)-1 to EXCLUDE the classfication {0,1}
		if (FRESTRICT == FEAT14):
			for i in range(0,14):
				distSq += (reliefWeightGlo[top14Index[i]] * \
					(pow(list1[top14Index[i]] - \
					list2[top14Index[i]], 2)))
		elif (FRESTRICT == FEATALL):
			for i in range(0,len(list1)-1):
				distSq += (reliefWeightGlo[i] * \
					    (pow(list1[i] - list2[i], 2)))
		return sqrt(distSq)

#trainGlobal2d structure is [dataLineNumber][featureNumber] = data (an int)
def saveTrainLine(featuresTrainingNum, line):
	# For a single call to saveTrainLine, 
	#  enough info to fill trainGlobal2d[x][0-13]
	# Start with baseline of # of dataLine's already saved
	dataLines = len(trainGlobal2d)
	# First extend the line number by one:
	trainGlobal2d.append([])
	#break up comma-delimited 'line' into list
	lineItems = lineToList(line)

	for featureNum in range(0,featuresTrainingNum):
		trainGlobal2d[dataLines].append(float(lineItems[featureNum]))

	#process until index featuresTrainingNum (featuresTrainingNum + 1 items)
	#  so that last entry is our **classification of {0,1}**
	trainGlobal2d[dataLines].append(int(lineItems[featuresTrainingNum]))
	
def saveTestLine(featuresTrainingNum, line):
        dataLines = len(testGlobal2d)
        testGlobal2d.append([])
        lineItems = lineToList(line)
        for featureNum in range(0,featuresTrainingNum):
                testGlobal2d[dataLines].append(float(lineItems[featureNum]))
        testGlobal2d[dataLines].append(int(lineItems[featuresTrainingNum]))

def classificationOfTrain(sampleIndex):
	return trainGlobal2d[sampleIndex][featuresTrainingNum]
def classificationOfTest(sampleIndex):
	return testGlobal2d[sampleIndex][featuresTrainingNum]

def findNearestNewTraining(sampleIndex, SAMPLEMODE, RELIEFMODE, FRESTRICT):
	bestDist = sys.maxint
	bestIndex = 0
	alreadyInK = 0
	# (start at range 0 because first element's distance init'd as best)
	for trainDataIndex in range(0,len(trainGlobal2d)):
		#always compare against the FIRST TEST LINE
		if (SAMPLEMODE == TESTMODE):
			if (RELIEFMODE == ONRELIEF):
				newDist = \
			WeightedReliefDist(trainGlobal2d[trainDataIndex],
						testGlobal2d[sampleIndex],
							FRESTRICT)
			elif (RELIEFMODE == OFFRELIEF):
				newDist = EucDist(trainGlobal2d[trainDataIndex],
						testGlobal2d[sampleIndex],
							FRESTRICT)
		elif (SAMPLEMODE == TRAINMODE):
			if (RELIEFMODE == ONRELIEF):
				newDist = \
			WeightedReliefDist(trainGlobal2d[trainDataIndex],
						trainGlobal2d[sampleIndex],
							FRESTRICT)
			elif (RELIEFMODE == OFFRELIEF):
				newDist = EucDist(trainGlobal2d[trainDataIndex],
						trainGlobal2d[sampleIndex],
							FRESTRICT)
		if (newDist < bestDist):
			alreadyInK = (trainDataIndex in kBest)
			if (not alreadyInK):
				bestDist = newDist
				bestIndex = trainDataIndex
	return bestIndex

def kBestClear():
	global kBest
	del kBest[:]
	kBest = []
def majorityVote():
	vote = 0
	halfK = (userGivenK / 2)
	if (userGivenK % 2):
		halfK += 1
	### THE SUM OF ALL DECISIONS (0 MEANS NO 1 MEANS YES)
	### ex. (k=5): IF THREE or more YESes (1s) AMONG 5 => MAJORITY IS YES
	### IF TWO or fewer YESes, THERE WERE AT LEAST THREE NOs (0s)\
	## COUNT UP THE VOTES, MAJORITY WINS (>= half is 1, < half is 0)
	for i in range(0,len(kBest)):
		if (classificationOfTrain(kBest[i]) == 1):
			vote += 1
	if (vote >= halfK):
		return 1
	else:
		return 0

def classifySamples(RELIEFMODE, FRESTRICT):
	goodMatch = 0
	badMatch = 0
	### CHECK EACH TEST SAMPLE
	for testSampleIndex in range(0,len(testGlobal2d)):
		kBestClear()

		### IN EACH TEST SAMPLE, FIND A NEW CLOSEST K TIMES
		for kSample in range(0,userGivenK):
			if (len(kBest) < userGivenK):

				### GO FIND A NEW NEIGHBOR
				#MODE: 0 if training set, 1 if test set
				kBest.append(findNearestNewTraining( \
					testSampleIndex, TESTMODE, RELIEFMODE,
						FRESTRICT))
		
		majorityVoteDecision = majorityVote()

		### CHECK IF OUR MAJORITY (kNN ALGORITHM) WAS CORRECT  ###
		if (majorityVoteDecision !=
			classificationOfTest(testSampleIndex)):
			badMatch += 1
		else:
			goodMatch += 1

########################### PRINTING ###############
###################################################
	print "%.1f (%d)"%(goodMatch,100*(goodMatch/float(goodMatch+badMatch)))

## ****************** RELIEF Functions **************** ##
def hitBool(pt1, pt2):
	return (classificationOfTrain(pt1) == classificationOfTrain(pt2))
def findFirstHit(sampleIndex):
	print "not yet implemented"
def kBestBoundedCheck():
	if (len(kBest) > len(trainGlobal2d)):
		print "Fatal - More kBest than training data exists"
		quit()	
	
## USE kBest TO ACCUMULATE ALL TESTED SAMPLES UNTIL WE FIND THE OPP HIT/MISS
def findFirstHit(sampleIndex):
	spotsLeft = len(trainGlobal2d) - 2	#(first 2: sample & miss)
	for i in range(0, spotsLeft):
		newNeighbor = findNearestNewTraining(sampleIndex, TRAINMODE, 
							OFFRELIEF, FEATALL)
		kBest.append(newNeighbor)
		kBestBoundedCheck()

		## CHECK IF THE NEXT NEIGHBOR HAS DIFF SAME AS x (x= kBest[0])
		if (hitBool(newNeighbor,kBest[0])):
			return newNeighbor
	## PERFECT DATA, NO MISSes.
	print "DEBUG: Overran some bounds!!"
	quit()
def findFirstMiss(sampleIndex):
	spotsLeft = len(trainGlobal2d) - 2
	for i in range(0, spotsLeft):
		newNeighbor = findNearestNewTraining(sampleIndex, TRAINMODE, 
							OFFRELIEF, FEATALL)
		kBest.append(newNeighbor)
		kBestBoundedCheck()
		if (not hitBool(newNeighbor,kBest[0])):
			return newNeighbor
	print "DEBUG: Overran some bounds!! (in MISS)"
	quit()

## RELIEF: (notes)
# Relief is a global list: reliefWeightGlo
# ---> list of i decimals / across i dimensions (i => number of featuers)
# - Calculate relief once per sample (do m samples total)
# - each time you do relief on a sample, you update the universal Wi
#   - therefore Wi is a sum of the weights, built by rand samples (ex: m = 1000)
def pickTop14():
	global top14, top14Index
	top14Index = []
        for i in range(0,14):
		best = -1
		top14Index.append(0)
		for feat in range(0,featuresTrainingNum):
			if ((reliefWeightGlo[feat] > best ) and \
				(feat not in top14Index) and \
				(len(top14Index) <= 14)):
				best = reliefWeightGlo[feat]
				top14Index[i] = feat
def calculateRelief(sampleIndex, xHit, xMiss):
	xHitDiff = []
	xMissDiff = []
	for i in range(0,featuresTrainingNum):
		diff = trainGlobal2d[sampleIndex][i] - trainGlobal2d[xHit][i]
		xHitDiff.append(fabs(diff))
		diff = trainGlobal2d[sampleIndex][i] - trainGlobal2d[xMiss][i]
		xMissDiff.append(fabs(diff))
		
		## SAVE RELIEF WEIGHT (ACROSS ALL FEATURES) INTO GLOBAL ARRAY
		weightCalc = reliefWeightGlo[i] - xHitDiff[i] + xMissDiff[i]
		if (weightCalc < 0):
			weightCalc = 0
		reliefWeightGlo[i] = weightCalc
 	pickTop14()

def prepareRelief(TESTRUNSIZE):
	## INITIALIZE reliefWeightGlo
	for i in range(0,featuresTrainingNum): 
		reliefWeightGlo.append(0)

	for i in range(0,TESTRUNSIZE):
		xRandSampleIndex = random.randint(0,len(trainGlobal2d)-1)

		## WE USE kBest FOR OUR RELIEF RANDOM CHOICE,
		##	SO THAT ITSELF IS NOT ITS CLOSEST NEIGHBOR
		kBestClear()			 ## RESETS kBest
		kBest.append(xRandSampleIndex)   ## BASELINE ==> kBest[0]

		## FIND FIRST NEAREST NEIGHBOR
		nearestToRandIndex = \
			findNearestNewTraining(xRandSampleIndex,
						TRAINMODE, OFFRELIEF, FEATALL)
		kBest.append(nearestToRandIndex) ## FIRST NEIGHBOR ==> kBest[1]

		## HIT or MISS ??
		if (hitBool(nearestToRandIndex,kBest[0])):  ## "HIT *****"
			xHit = nearestToRandIndex
			xMiss = findFirstMiss(xRandSampleIndex)
		else:					    ## "**** MISS"
			xMiss = nearestToRandIndex
			xHit = findFirstHit(xRandSampleIndex)

		calculateRelief(xRandSampleIndex,xHit, xMiss)
def arffDataReader(fileName):
	#can count # of attribute lines with 'NUMERIC' 
	#  (and not aligned well for other ex's of input)
	# since the only non-NUMERIC attributes are at the
	#   end of the list of attributes
	count = 0
	attNumericCount = 0     
	dataStart = 0
	testLineBuilder = []
	global featuresTrainingNum

	with open(fileName) as f:
		for line in f:
			count += 1
			if (dataStart):
				if "train" in fileName:
					saveTrainLine(featuresTrainingNum, line)
				else:
					saveTestLine(featuresTrainingNum, line)
			else:
				######## Skip nonData lines from .arff ########
				if 'NUMERIC' in line:
					attNumericCount += 1
				elif '@DATA' in line:
					featuresTrainingNum = attNumericCount
					dataStart = 1
				##############################################
def printUsage():
	print "usage: %s train_data_fil test_data_file k" % (sys.argv[0])
	quit()
def init():
	argcount = 0
	global trainFile, testFile, userGivenK
	if (len(sys.argv) != 4):
		printUsage()
	else:
		trainFile = sys.argv[1]
		testFile = sys.argv[2]
		userGivenK = int(sys.argv[3])
		# Handle: relative path 
		if (trainFile[0] != '/'):
			trainFile = os.getcwd() + "/" + trainFile
		if (testFile[0] != '/'):
			testFile = os.getcwd() + "/" + testFile
		if (userGivenK < 1):
			printUsage()
		if (os.path.isfile(trainFile) and os.path.isfile(testFile)):
			pass
		else:
			print "not all files indicated exist"
			printUsage()
################## START EXECUTION ##############
init()
arffDataReader(trainFile)
arffDataReader(testFile)

	## TO REMOVE HEADER, comment out single [long] line below:
print("Feature count=%d with k=%d. Results: weka J48 Training\nAccuracy [samples correct (percentage)]; weka J48 Test Accur; kNN;\nm (for Relief): {Relief (w/ distance weighting); Relief (14 feature selection)}" % (featuresTrainingNum, userGivenK))

runWeka()
classifySamples(OFFRELIEF, FEATALL)

	##CHANGE HERE FOR Question 3.4.2 - Toggle comment:
Q342 = [1000]
#Q342 = [100,200,300,400,500,600,700,800,900,1000]
for mrun in Q342:
	print "m=%d"%mrun
	prepareRelief(mrun)
	classifySamples(ONRELIEF, FEATALL)
	classifySamples(OFFRELIEF, FEAT14)
