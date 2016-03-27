import serial

def waitForOK(portname):
	done = 0;
	while (done == 0):
		x = portname.readline();
		print(" waitForOK: Read " + str(repr(x)));
		if (x == 'OK\r\n'):
			done = 1;

#	done = 0;
#	while (done == 0):
#		x = portname.readline();
#		if (x == '> '):
#			done = 1;



port = serial.Serial(
	port='/dev/ttyACM0',
	baudrate = 9600,
	parity = serial.PARITY_NONE,
	stopbits = serial.STOPBITS_ONE,
	bytesize = serial.EIGHTBITS,
	timeout = 1
	)

#port.write('L 1\n\r');
while True:
	x = port.readline();	
	print(repr(x));

	waitForOK(port);
	port.write('L 1\n');

