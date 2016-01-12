// SSD1351.cpp

//#include <plib.h>
//#include <wprogram.h>

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>


#include "SSD1351.h"


// Packs RGB values into a 16-bit RGB565 representation.  r, g, and b are between 255 (max) and 0 (min)
uint16_t RGB(uint16_t r, uint16_t g, uint16_t b) {
  uint16_t c = 0;
  c += (r >> 3) << 11;
  c += (g >> 2) << 5;
  c += b >> 3;
  return c;
}
// Decompose a packed RGB565 representation into its component colors (scaled to original 255-0, but last 2-3 bits will be 0)
uint8_t getRed(uint16_t col) {
  return (col >> 11) << 3;
}
uint8_t getGreen(uint16_t col) {
  return ((col & 0x07E0) >> 5) << 2;
}
uint8_t getBlue(uint16_t col) {
  return (col & 0x001F) << 3;
}

uint16_t HueToRGB(float h) {
  // Adapted from Schaller's course notes from the Rochester Institute of Technology
  // Hue: 0-359, Saturation and brightness both assumed to be 1
  // TODO: Make entirely integer arithmetic
  
  h /= 60;			// sector 0 to 5  
  int i = floor( h );
  int f = 255 * (h - i);	// factorial part of h
  
  float q = ( 255 - (255 * f) );
  float t = ( 255 - ( 255 - f ) );
  uint16_t p0 = 0;
  uint16_t q0 = floor(q);
  uint16_t t0 = floor(t);  
  uint16_t v0 = 255;
  
  switch( i ) {
    case 0:
      return RGB(v0, t0, p0);
    break;
    case 1:
      return RGB(q0, v0, p0);
    break;
    case 2:
      return RGB(p0, v0, t0);
    break;
    case 3:
      return RGB(p0, q0, v0);
    break;
    case 4:
      return RGB(t0, p0, v0);
    break;
    default:		// case 5:
      return RGB(v0, p0, q0);
    break;
  }
  
  return 0;
}



void SSD1351::writeCommand(uint8_t c) {
	int result = 0;	
	
	// Set data/command pin to LOW (command)
	digitalWrite(SSD1351_DC, 0);
	
	// Send data
	buffer[0] = c;
	wiringPiSPIDataRW(SSD1351_SPI_CHAN, buffer, 1);
	
/*	
  while(mIsPMPBusy());                // Wait for PMP to be free
  //digitalWrite(SSD1351_DC, 0);        // Low -- command mode 
  mPORTCClearBits(OLED_DC);           // Low -- command mode 
  
//  PMPMasterWrite( c );                // write character   

  mPORTEWrite( (uint16_t)c );    // Setup data
  mPORTDClearBits(OLED_RW);      // RW# 0 (write mode)
  mPORTDSetBits(OLED_EN);      // EN 1 (enable)
  mPORTDClearBits(OLED_CS);      // CS active   
//  delay(1);
  mPORTDSetBits(OLED_CS);      // CS inactive  
  mPORTDClearBits(OLED_EN);      // EN 0 (disable) 
*/
  
}

void SSD1351::writeData(uint8_t c) {
	int result = 0;
	
	// Set data/command pin to HIGH (data)
	digitalWrite(SSD1351_DC, 1);
		
	// Send data
	buffer[0] = c;
	wiringPiSPIDataRW(SSD1351_SPI_CHAN, buffer, 1);
	
/*	
  while(mIsPMPBusy());                // Wait for PMP to be free
//  digitalWrite(SSD1351_DC, 1);        // High -- data mode 
  mPORTCSetBits(OLED_DC);           // Hight -- command mode 

//  PMPMasterWrite( c );                // write character

  mPORTEWrite( (uint16_t)c );    // Setup data
  mPORTDClearBits(OLED_RW);      // RW# 0 (write mode)
  mPORTDSetBits(OLED_EN);      // EN 1 (enable)
  mPORTDClearBits(OLED_CS);      // CS active   
//  delay(1);
  mPORTDSetBits(OLED_CS);      // CS inactive  
  mPORTDClearBits(OLED_EN);      // EN 0 (disable) 
*/
}


// OLED 13V Boost regulator
void SSD1351::enableBoostRegulator() {
 // digitalWrite(SSD1351_BOOST_EN, 1);
}

void SSD1351::disableBoostRegulator() {
 // digitalWrite(SSD1351_BOOST_EN, 0);
}


// Initialization
void SSD1351::begin(void) {
    
  // Setup Wiring/GPIO
  wiringPiSetupGpio();	// Initialize wiringPi, using Broadcom pin numbers
  
  // Set pin tristates
  pinMode(SSD1351_DC, OUTPUT);
  pinMode(SSD1351_RESET, OUTPUT);
  
  digitalWrite(SSD1351_DC, 1);
  digitalWrite(SSD1351_RESET, 1);
  
  delay(500);
  
  // Disable OLED 13V boost supply 
  //Serial.println (" * Disabling Boost Regulator");
  //disableBoostRegulator();
  
  // Initialize SPI interface
  printf (" * Initializing SPI interface \n");  
  fd = wiringPiSPISetup(SSD1351_SPI_CHAN, SPI_SPEED);
  
  printf (" * Initialization result: %i \n", fd);
    
  
  // Reset display
  printf (" * Resetting display \n");  
  digitalWrite(SSD1351_RESET, 1); 
  delay(500);
  digitalWrite(SSD1351_RESET, 0); 
  delay(500);
  digitalWrite(SSD1351_RESET, 1); 
  delay(500);


    // Initialization Sequence
    printf (" * Beginning initialization sequence \n");
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0x12);  
    writeCommand(SSD1351_CMD_COMMANDLOCK);  // set command lock
    writeData(0xB1);

    writeCommand(SSD1351_CMD_DISPLAYOFF);  		// 0xAE

    writeCommand(SSD1351_CMD_CLOCKDIV);  		// 0xB3
    writeCommand(0xF1);  						// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    
    writeCommand(SSD1351_CMD_MUXRATIO);
    writeData(127);
    
    writeCommand(SSD1351_CMD_SETREMAP);
    writeData(0x74);
  
    writeCommand(SSD1351_CMD_SETCOLUMN);
    writeData(0x00);
    writeData(0x7F);
    writeCommand(SSD1351_CMD_SETROW);
    writeData(0x00);
    writeData(0x7F);

    writeCommand(SSD1351_CMD_STARTLINE); 		// 0xA1
    if (SSD1351_HEIGHT == 96) {
      writeData(96);
    } else {
      writeData(0);
    }


    writeCommand(SSD1351_CMD_DISPLAYOFFSET); 	// 0xA2
    writeData(0x0);

    writeCommand(SSD1351_CMD_SETGPIO);
    writeData(0x00);
    
    writeCommand(SSD1351_CMD_FUNCTIONSELECT);
    writeData(0x01); // internal (diode drop)
    //writeData(0x01); // external bias

//    writeCommand(SSSD1351_CMD_SETPHASELENGTH);
//    writeData(0x32);

    writeCommand(SSD1351_CMD_PRECHARGE);  		// 0xB1
    writeCommand(0x32);
 
    writeCommand(SSD1351_CMD_VCOMH);  			// 0xBE
    writeCommand(0x05);

    writeCommand(SSD1351_CMD_NORMALDISPLAY);  	// 0xA6

    writeCommand(SSD1351_CMD_CONTRASTABC);
    writeData(0xC8);
    writeData(0x80);
    writeData(0xC8);

    writeCommand(SSD1351_CMD_CONTRASTMASTER);
    writeData(0x0F);

    writeCommand(SSD1351_CMD_SETVSL );
    writeData(0xA0);
    writeData(0xB5);
    writeData(0x55);
    
    writeCommand(SSD1351_CMD_PRECHARGE2);
    writeData(0x01);
   
       printf (" * Turning display on \n");    
    writeCommand(SSD1351_CMD_DISPLAYON);		//--turn on oled panel    
    
    // Clear framebuffer
    for (int i=0; i<(width*height); i++) {
      framebuffer[i] = 0;
    }
    
    // Update screen
    updateScreen();   

    // Enable OLED high voltage supply
    //Serial.println (" * Enabling boost regulator");
    //enableBoostRegulator();    
	delay(100);
	

   
}

// Constructor
SSD1351::SSD1351() {
}

// Use
void SSD1351::TestPattern(uint8_t mode) {
    // Simple test pattern -- fade white in and out
    int delayTime = 1;
    int colors[4] = { RGB(255, 255, 255), RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255) };
            
    for (int i=0; i<4; i++) {        
		// Display test pattern
//		for (uint16_t c=0; c<255; c+= 32) {
			fillRect(0, 0, 127, 127, colors[i]);
			updateScreen();
			delay(250);
//		}
	}
}

uint16_t SSD1351::RGB565(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c;
  c = r >> 3;
  c <<= 6;
  c |= g >> 2;
  c <<= 5;
  c |= b >> 3;

  return c;
}

void SSD1351::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) 
{	

  // Bound checking
  if ((x >= width) || (y >= height)) return;
  if (y+h > height) h = height;
  if (x+w > width)  w = width - x - 1;
  
/*
  Serial.print(x); Serial.print(", ");
  Serial.print(y); Serial.print(", ");
  Serial.print(w); Serial.print(", ");
  Serial.print(h); Serial.println(", ");
*/

/*
  // set location
  writeCommand(SSD1351_CMD_SETCOLUMN);
  writeData(x);
  writeData(x+w-1);
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(y+h-1);
  // fill!
  writeCommand(SSD1351_CMD_WRITERAM);  

  for (uint16_t i=0; i < w*h; i++) {
    writeData(fillcolor >> 8);
    writeData(fillcolor);
  }
  */
  for (uint16_t hIdx=0; hIdx<h; hIdx+=1) {
    uint32_t fbIdx = fbY(y + hIdx);
    for (uint16_t wIdx=0; wIdx<w; wIdx+=1) {
      framebuffer[fbIdx] = color;
      fbIdx += 1;
    }      
  }
  
}


// Fast methods for finding a position in the framebuffer
uint32_t SSD1351::fbXY(uint16_t x, uint16_t y) {  
  return ((y * height) + x);
}

uint32_t SSD1351::fbY(uint16_t y) {
  return (y * height);
}


// Sets the (x, y) location in memory on the physical device (not the framebuffer)
void SSD1351::setXY(uint8_t x, uint8_t y) {
  // Bound checking
  if ((x >= width) || (y >= height)) return;
  
  // Set X location
  writeCommand(SSD1351_CMD_SETCOLUMN);  
  writeData(x);
  writeData(width-1);
  
  // Set Y Location
  writeCommand(SSD1351_CMD_SETROW);
  writeData(y);
  writeData(height-1);  
  
  // Setup SSD1351 for receiving pixel data
  writeCommand(SSD1351_CMD_WRITERAM);    
}


// Flips the backbuffer -- transfers the framebuffer contents to the actual display. 
void SSD1351::updateScreen() {  

	printf("    * Updating screen...\n");
	// Set location to (0, 0)
	setXY(0, 0);

	printf("    * Transferring buffer..\n");
  
	// Transfer contents of framebuffer to SSD1351 display
	uint8_t displayOut[10000];
	int idx = 0;

	// Set data/command pin to HIGH (data)
	digitalWrite(SSD1351_DC, 1);

	// Bulk transfer, line-by-line
	for (int y=0; y<height; y++) {
		idx = 0;
		for (int x=0; x<width; x++) {		  
			uint16_t pixel = framebuffer[(height*y)+x];
			displayOut[idx] = ((pixel >> 8) & 0x00FF);
			displayOut[idx+1] = (pixel & 0x00ff);
			idx += 2;
		}
		wiringPiSPIDataRW(SSD1351_SPI_CHAN, displayOut, width*2);
	}  
  
  printf("    * Transfer complete... \n");
   
}
