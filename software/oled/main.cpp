// test of the wiring library


#include <stdio.h>
#include <wiringPi.h>

#include "SSD1351.h"


const int ledPin = 26;
const int buttonPin = 22;

SSD1351 Framebuffer;

int main(void) {
	
	printf(" * blinker: Initializing...\n");
	
	// Setup Wiring/GPIO
	wiringPiSetupGpio();	// Initialize wiringPi, using Broadcom pin numbers
	
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT);

/*
	printf(" * blinking once... \n");
			digitalWrite(ledPin, HIGH);
			delay(75);
			digitalWrite(ledPin, LOW);
			delay(75);
*/

	printf(" * Initializing display... \n");
	Framebuffer.begin();
	
	Framebuffer.fillRect(0, 0, 32, 32, RGB(0, 255, 0));
	Framebuffer.updateScreen();
	
	
	printf(" * Displaying test pattern... \n");
	for (uint8_t i=0; i<8; i++) {
		Framebuffer.TestPattern(i);
	}
	

	/*
	printf(" * blinker is running.  Press CTRL+C to quit.\n");
	
	while (1) {
		
		if (digitalRead(buttonPin)) {
			// Button released if this returns 1
			digitalWrite(ledPin, LOW);
		} else {
			// Button pressed
			
			digitalWrite(ledPin, HIGH);
			delay(75);
			digitalWrite(ledPin, LOW);
			delay(75);
		}
	}
	*/
	
}
