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
def waitForResponse(portname):
	response = [];
	done = 0;
	while (done == 0):
		x = portname.readline();
		#print(" waitForOK: Read " + str(repr(x)));
		if (x == 'OK\r\n'):
			done = 1;			
		else:
			if (len(x) > 0):
				response.append(x);
			
			
	
	return response;



#
# Imaging array commands
# 
def setDigipotsAll(ser, value):
	ser.write('A ' + str(value) + '\n');
	resp = waitForResponse(ser);
	

def sampleDetectors(ser,  intTime):
	ser.write('I ' + str(intTime) + '\n');
	resp = waitForResponse(ser);
	
	measurements = re.split(r'\t', resp[1].rstrip());
	return measurements
	

#
# Export
#
def exportHeader():
	print("z\t", end = "")
	print("digipot\t", end = "")
	print("inttime\t", end = "")
	for i in range(0, 16):
		print("det" + str(i) +"\t", end = "")
		
	print("")

def exportMeasurements(z, digipotVal, intTime, measurements):
	print(z, end = "");
	print("\t", end = "");
	print(digipotVal, end = "");
	print("\t", end = "");
	print(intTime, end = "");
	print("\t", end = "");
	
	measurementsStr = '\t'.join(str(x) for x in measurements)
	print(measurementsStr, end = "");
	
	print("");
	



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
	resp = waitForResponse(serImager);
	print(resp)


	# Display header
	exportHeader();
	
	numIter = intTime/intChunk
	digipotDeltaSign = digipotDelta / abs(digipotDelta)
	digipotEnd = digipotStart + (digipotDelta * digipotSteps)
	
	for digipotCurrent in range(digipotStart, digipotEnd, digipotDelta):
		# Set Imager digipots
		setDigipotsAll(serImager, digipotCurrent);

		for iter in range(0, numIter):	
			sleep(0.5)
			measurements = sampleDetectors(serImager, intChunk);	
			exportMeasurements(zLoc, digipotCurrent, intChunk, measurements);


	# Close
	serStepper.close()
	serImager.close()


# Main Program
if __name__ == "__main__":
	main(sys.argv[1:])
