from __future__ import print_function
import sys, getopt
import re
#import serial
import argparse
from time import sleep
from collections import defaultdict



#
# Read file
#
def summarizeFile(filename):
	
	header = []
	detectorMap = {}
	
	lineCount = 0	
	counts = defaultdict(lambda: 0)

	zLocs = set()
	rLocs = set()
	dets = set()

	with open(filename, 'r') as f:
		for line in f:
			tabFields = re.split(r'\t', line.rstrip())
			
			if (lineCount == 0):
				header = tabFields
				detectorMap = parseHeader(tabFields)
			else:
				zLoc = tabFields[0]
				rLoc = tabFields[1]
				intTime = tabFields[2]

				for detKey in detectorMap:
					#if not (zLoc, rLoc, detKey) in counts:
					#	counts[(zLoc, rLoc, detKey)] = defaultdict(int)

					zLocs.add(zLoc)
					rLocs.add(rLoc)
					dets.add(detKey)
					counts[(zLoc, rLoc, detKey)] += int( tabFields[detectorMap[detKey]] )

				counts[(zLoc, rLoc, "inttime")] += int( intTime )

			lineCount += 1

	return (zLocs, rLocs, dets, counts)
				

def parseHeader(tabFields):
	print("Tab Fields: " + str(tabFields))
	detectorMap = {}
	for i in range(0, len(tabFields)):
		if (tabFields[i].startswith('det')):
			detNum = int(tabFields[i][3:])
			detectorMap[detNum] = i
			
	return detectorMap
	
	
#
# Display
#

def displaySummary(zLocs, rLocs, dets, counts):
	# Header
	print('zLoc\trLoc\t', end = "")
	for i in range(0, 16):
		print('det' + str(i) + '\t', end = "")
	print("")

	zLocsSorted = sorted(zLocs)
	rLocsSorted = sorted(rLocs)
	detsSorted = sorted(dets)

	# Data
	for zLoc in zLocsSorted:
		for rLoc in rLocsSorted:
			print(str(zLoc) + "\t", end = "")
			print(str(rLoc) + "\t", end = "")
			for key2 in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]:
				intTime = float(counts[(zLoc, rLoc, "inttime")])
				count = float(counts[(zLoc, rLoc, key2)])
				sampleTimeMins = float(intTime) / 60.0
				countsPerMinute = count / sampleTimeMins
				cpmStr = "{:0.1f}".format(countsPerMinute)
				print(cpmStr + '\t', end = "")
			print("")
	

#
#
# Main Program
#

def main(argv):
	
	# Default Values
	inputFilename = ""
	
	# Parse command line arguments
	parser = argparse.ArgumentParser(description='description');
	parser.add_argument('-i', '--inputfilename', help="Input file of detector samples", required = True);
	
	args = vars(parser.parse_args())
	print("Args: " + str(args))
	
	if (args['inputfilename'] != None):
		inputFilename = args['inputfilename']
			



	print("Started...")
	(zLocs, rLocs, dets, counts) = summarizeFile(inputFilename)
	displaySummary(zLocs, rLocs, dets, counts)
	

# Main Program
if __name__ == "__main__":
	main(sys.argv[1:])
