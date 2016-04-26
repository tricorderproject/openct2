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
	counts = {}
		
	with open(filename, 'r') as f:
		for line in f:
			tabFields = re.split(r'\t', line.rstrip())
			
			if (lineCount == 0):
				header = tabFields
				detectorMap = parseHeader(tabFields)
			else:
				digipotVal = tabFields[1]
				intTime = tabFields[2]
				
				if not digipotVal in counts:
					counts[digipotVal] = defaultdict(int)
					
				for key in detectorMap:
					counter = counts[digipotVal]
					counter[key] += int( tabFields[detectorMap[key]] )
					
				counter['inttime'] += int(intTime)
				
			lineCount += 1
				
	return counts
				

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

def displaySummary(counts):
	# Header
	print('inttime\t', end = "")
	for i in range(0, 16):
		print('det' + str(i) + '\t', end = "")
	print("")
	
	# Data
	for key in sorted(counts):
		print(str(key) + '\t', end = "")
		intTime = counts[key]['inttime']
		for key2 in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]:
			count = float(counts[key][key2])			
			sampleTimeMins = float(intTime) / 60.0
			countsPerMinute = count / sampleTimeMins
			cpmStr = "{:1}".format(countsPerMinute)
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
	counts = summarizeFile(inputFilename)
	displaySummary(counts)
	

# Main Program
if __name__ == "__main__":
	main(sys.argv[1:])
