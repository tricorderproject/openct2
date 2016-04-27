from __future__ import print_function
import sys, getopt
import re
import serial
import argparse
from time import sleep

#
# Globals
#
currentZLocation = -1
currentRLocation = -1



#
# Serial Port Initialization
# 
def initializeSerial(deviceLoc):
	port = serial.Serial(
		port=deviceLoc,
		baudrate = 9600,
		parity = serial.PARITY_NONE,
		stopbits = serial.STOPBITS_ONE,
		bytesize = serial.EIGHTBITS,
		timeout = 1
		)
		
	return port;


#
# Serial Port I/O
#
def waitForResponse(portname, timeoutSec = 10):
	response = []
	done = 0
	error = 0
	numSec = 0
	while (done == 0):
		x = portname.readline();
		print(" waitForOK: Read " + str(repr(x)));
		if (x == 'OK\r\n'):
			done = 1		
		elif (x == 'UNRECOGNIZED INPUT\r\n'):
			error = 1
			done = 1
		else:
			if (len(x) > 0):
				response.append(x);
		
		# Timeout (in seconds)		
		if (numSec > timeoutSec):
			error = 1
			done = 1
			
		numSec += 1
			
			
	
	return (error, response)



def sendCommand(ser, command, timeoutSec = 10, maxRetries = 10):
	resp = []
	errorCode = 1
	numRetries = 0
	while ((errorCode != 0) and (numRetries < maxRetries)): 
		ser.write(command)
		sleep(0.5)

		(errorCode, resp) = waitForResponse(ser, timeoutSec)
		if (errorCode == 1):
			print("Communications Error: Retrying...")
			sleep(1.0)
			numRetries += 1

	return (errorCode, resp)
	 

#
# Motion platform commands
#
def homeZAxis(ser):
	global currentZLocation
	
	print("Homing Axis...")
	command = 'H 1\n'
	(errorCode, resp) = sendCommand(ser, command, 30)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)	
	currentZLocation = 0

def moveZAxisRelative(ser, value):
	global currentZLocation
	
	command = 'Z ' + str(value) + '\n'
	(errorCode, resp) = sendCommand(ser, command, 10)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)
	currentZLocation += value
	
def moveZAxisAbsolute(ser, value):
	global currentZLocation
	
	delta = value - currentZLocation
	
	while (delta != 0):
		# Direction
		direction = 1
		if (delta < 0):
			direction = -1
			
		# Magnitude
		if (abs(delta) > 200):
			ammountToMove = 200
		else:
			ammountToMove = abs(delta)
			
		ammountToMove *= direction
		
		# Move
		moveZAxisRelative(ser, ammountToMove)

		# Recalculate delta
		delta = value - currentZLocation
			


#
# Imaging array commands
# 
def setDigipotsAll(ser, value):
	command = 'A ' + str(value) + '\n'
	(errorCode, resp) = sendCommand(ser, command, 5)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)
	

def setDigipotChannel(ser, channel, value):
	command = 'D ' + str(channel) + ' ' + str(value) + '\n'
	(errorCode, resp) = sendCommand(ser, command, 5)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)
	
	
# Note: 'calib' is a dictionary of [int(channel)] = value(digipotValue) tuples
def setDigipotFromCalibration(ser, calib):
	for i in range(0, 16):
		channel = i
		digipotVal = calib[channel]
		setDigipotChannel(ser, channel+1, digipotVal)		
	

def sampleDetectors(ser,  intTime):
	command = 'I ' + str(intTime) + '\n'
	(errorCode, resp) = sendCommand(ser, command, intTime+5)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)
	
	measurements = re.split(r'\t', resp[1].rstrip());
	return measurements
	

#
# Export
#
def exportHeader():
	os = ""
	os += "z\t"
	os += "digipot\t"
	os += "inttime\t"
	for i in range(0, 16):
		os += "det" + str(i) +"\t"
	
	return os
	

def exportMeasurements(z, r, intTime, measurements):
	os = ""
	os += str(z) + "\t"
	os += str(r) + "\t"
	os += str(intTime) + "\t"
	
	measurementsStr = '\t'.join(str(x) for x in measurements)
	os += measurementsStr
	
	return os
	

#
# Input from file
#
def readDigipotCalibration(filename):
	digipotValues = {}
	with open(filename, 'r') as f:
		for line in f:
			tabFields = re.split(r'\t', line.rstrip())
		
			for i in range(0, 16):
				digipotValues[i] = int(tabFields[i])

	print("Read digipot calibration: " + str(digipotValues))
	return digipotValues
	
	

#
# Supporting functions
#
def printUsage():
	print("acquire.py -z <zlevel> -inttime <int> -intchunk <int>");
	#print("");
	#print("Example: ")
	#print("test.py -z 0 -scanstart 175 -scanstep 5 -inttime 30 -intchunk 5");
	#print("Procedure: ")
	#print(" 1. Moves to z = 0")
	#print(" 2. Sets detector digipots to 175")
	#print(" 3. Integrate for 30 seconds, recording value every 5 seconds")
	#print(" 4. Once complete, decrement digipots by 1, repeat at step 3. ")
	#print("    Continue for 5 iterations (scanstep), until digipot value is 170.")
	

#
# Main Program
#

def main(argv):
	
	# Default Values
	zStart = 0
	zSteps = 1
	zDelta = 0
	digipotStart = 175
	digipotDelta = -1
	digipotSteps = 1
	intTime = 30
	intChunk = 5
	deviceSerialStepper = '/dev/ttyACM0'
	deviceSerialImager = '/dev/ttyUSB0'
	
	
	
	# Parse command line arguments
	parser = argparse.ArgumentParser(description='description');
	parser.add_argument('-z', '--zlocation', help="Move to this Z axis level before scan", required = True);
	parser.add_argument('-zs', '--zsteps', help="Number of positions to measure on the Z axis", required = False)
	parser.add_argument('-zd', '--zdelta', help="Distance (in steps) to move Z axis after each position", required = False)	
	parser.add_argument('-idc', '--digipotcalibfile', help="Digipot calibration file (tab delimited)", required = True);
	parser.add_argument('-it', '--inttime', help="Total integration time per digipot value", required = False);
	parser.add_argument('-ic', '--intchunk', help="Total integration time is divided into chunks of this many seconds", required = False);	
	parser.add_argument('-o', '--outfile', help="Output filename", required = True);
	
	args = vars(parser.parse_args())
	print("Args: " + str(args))
	
	if (args['zlocation'] != None):
		zStart = int( args['zlocation'] )		
	if (args['digipotcalibfile'] != None):
		digipotCalibFilename = args['digipotcalibfile']	
	if (args['inttime'] != None):
		intTime = int( args['inttime'] )
	if (args['intchunk'] != None):
		intChunk = int( args['intchunk'] )
	if (args['outfile'] != None):
		outputFilename = args['outfile']
		
	if (args['zsteps'] != None):
		zSteps = int(args['zsteps'])
	if (args['zdelta'] != None):
		zDelta = int(args['zdelta'])
		
			


	# Initialize serial devices
	serStepper = initializeSerial(deviceSerialStepper);
	serImager = initializeSerial(deviceSerialImager);

	print("Started...")
	# Clear buffer
	(errorCode, resp) = waitForResponse(serStepper);
	print(resp)
	(errorCode, resp) = waitForResponse(serImager);
	print(resp)

	# Open output file
	fp = open(outputFilename, 'w')

	# Display header
	print( exportHeader() )
	print( exportHeader(), file = fp )

	# Home Z Axis
	homeZAxis(serStepper)

	# Set digipot values for each channel
	print(" Setting Digipot Values...")
	digipotCalib = readDigipotCalibration(digipotCalibFilename)
	setDigipotFromCalibration(serImager, digipotCalib)	
		
	# Read data
	numIter = intTime/intChunk
	for zIter in range(0, zSteps):
		newZLoc = zStart + (zDelta * zIter)
		print("Moving to Z = " + str(newZLoc))
		moveZAxisAbsolute(serStepper, newZLoc)
		
		for iter in range(0, numIter):	
			sleep(0.5)
			measurements = sampleDetectors(serImager, intChunk)	
			print( exportMeasurements(currentZLocation, currentRLocation, intChunk, measurements) )
			print( exportMeasurements(currentZLocation, currentRLocation, intChunk, measurements), file = fp )
			fp.flush();


	# Close
	serStepper.close()
	serImager.close()

	fp.close()

# Main Program
if __name__ == "__main__":
	main(sys.argv[1:])
