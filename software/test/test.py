from __future__ import print_function
import sys, getopt
import re
import serial
import argparse
from time import sleep



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
# Imaging array commands
# 
def setDigipotsAll(ser, value):
	command = 'A ' + str(value) + '\n'
	(errorCode, resp) = sendCommand(ser, command, 5)
	if (errorCode != 0):
		print("Communications error: exiting...")
		sys.exit(1)
	
	

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
	

def exportMeasurements(z, digipotVal, intTime, measurements):
	os = ""
	os += str(z) + "\t"
	os += str(digipotVal) + "\t"
	os += str(intTime) + "\t"
	
	measurementsStr = '\t'.join(str(x) for x in measurements)
	os += measurementsStr
	
	return os
	



#
# Supporting functions
#
def printUsage():
	print("test.py -z <zlevel> -scanstart <255-0> -scanstep <0-255> -inttime <int> -intchunk <int>");
	print("");
	print("Example: ")
	print("test.py -z 0 -scanstart 175 -scanstep 5 -inttime 30 -intchunk 5");
	print("Procedure: ")
	print(" 1. Moves to z = 0")
	print(" 2. Sets detector digipots to 175")
	print(" 3. Integrate for 30 seconds, recording value every 5 seconds")
	print(" 4. Once complete, decrement digipots by 1, repeat at step 3. ")
	print("    Continue for 5 iterations (scanstep), until digipot value is 170.")
	

#
# Main Program
#

def main(argv):
	
	# Default Values
	zLoc = 0
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
	parser.add_argument('-dst', '--digipotstart', help="Start digipot at this value", required = True);
	parser.add_argument('-dss', '--digipotsteps', help="Scan digipot value through this many values", required = False);
	parser.add_argument('-dd', '--digipotdelta', help="Delta value to scan digipot each step", required = False);
	parser.add_argument('-it', '--inttime', help="Total integration time per digipot value", required = False);
	parser.add_argument('-ic', '--intchunk', help="Total integration time is divided into chunks of this many seconds", required = False);
	
	
	args = vars(parser.parse_args())
	print("Args: " + str(args))
	
	if (args['zlocation'] != None):
		zLoc = int( args['zlocation'] )		
	if (args['digipotstart'] != None):
		digipotStart = int( args['digipotstart'] )	
	if (args['digipotsteps'] != None):
		digipotSteps = int( args['digipotsteps'] )
	if (args['digipotdelta'] != None):
		digipotDelta = int( args['digipotdelta'] )
	if (args['inttime'] != None):
		intTime = int( args['inttime'] )
	if (args['intchunk'] != None):
		digipotSteps = int( args['intchunk'] )
			


	# Initialize serial devices
	serStepper = initializeSerial(deviceSerialStepper);
	serImager = initializeSerial(deviceSerialImager);

	print("Started...")
	# Clear buffer
	(errorCode, resp) = waitForResponse(serImager);
	print(resp)

	# Open output file
	fp = open('out.txt', 'w')

	# Display header
	print( exportHeader() )
	print( exportHeader(), file = fp )
	
	numIter = intTime/intChunk
	digipotDeltaSign = digipotDelta / abs(digipotDelta)
	digipotEnd = digipotStart + (digipotDelta * digipotSteps)
	
	for digipotCurrent in range(digipotStart, digipotEnd, digipotDelta):
		# Set Imager digipots
		setDigipotsAll(serImager, digipotCurrent);

		for iter in range(0, numIter):	
			sleep(0.5)
			measurements = sampleDetectors(serImager, intChunk)	
			print( exportMeasurements(zLoc, digipotCurrent, intChunk, measurements) )
			print( exportMeasurements(zLoc, digipotCurrent, intChunk, measurements), file = fp )
			fp.flush();


	# Close
	serStepper.close()
	serImager.close()

	fp.close()

# Main Program
if __name__ == "__main__":
	main(sys.argv[1:])
