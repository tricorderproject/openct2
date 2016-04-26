// OpenCT2 Imaging Array Controller/Parser
// Peter Jansen, October/2015
// 
// Usage: Accepts serial commands of the form <char> <int_argument> <optional int_argument>, 
// Where: A <1-255> sets the digipot threshold for all channels
// D <1-16> <1-255> sets the digipot threshold for a particular channel (1-16)
// I <1-600> measures/integrates from all channels for 1-600 seconds, and displays the results
//
// Requires: Adafruit MCP23017 and MCP23008 I/O Expander Libraries

#include <Wire.h>
#include "Adafruit_MCP23017.h"
#include "Adafruit_MCP23008.h"

// Global objects
Adafruit_MCP23017 IOExpander;       // I/O expander for detector CS lines (on imaging array board)
Adafruit_MCP23008 IOExpanderSPI;    // I/O expander for SPI lines (on microcontroller board)

// Sensor channels
#define MAX_CHANNELS   16
#define MAX_HISTBINS   10

// Sensor data
uint16_t channelCounts[MAX_CHANNELS]; 
long unsigned int channelLastTimes[MAX_CHANNELS];
long unsigned int channelStartTimes[MAX_CHANNELS];
uint16_t channelHist[MAX_CHANNELS][MAX_HISTBINS]; 

#define MAX_COUNTS_PER_CHANNEL  64000
 
// Radiation sensor detections are active high 
#define SENSOR_ACTIVE   1
#define SENSOR_INACTIVE 0

// Channel debounce time (in microseconds).  No counts will be made if the last count was within this time. 
#define SENSOR_DEBOUNCETIME  150

// Pin Definitions (SPI/Digipot).  These are pin numbers on the MCP23008 I/O expander
#define DIGIPOT_MOSI  0 
#define DIGIPOT_SCLK  1

// Pin Definitions (Sensor Channels)
#define CH0    A2
#define CH1    A3
#define CH2    A0
#define CH3    A1
#define CH4    12
#define CH5    13
#define CH6    10
#define CH7    11
#define CH8    3
#define CH9    2
#define CH10   5
#define CH11   4
#define CH12   7
#define CH13   6
#define CH14   9 
#define CH15   8

// Port/Pin definitions for interrupt map
// PB0(8), PB1(9), PB2(10), PB3(11), PB4(12), PB5(13) 
#define chPB0  15
#define chPB1  14
#define chPB2  6
#define chPB3  7
#define chPB4  4
#define chPB5  5


// PC0(AD0), PC1(AD1), PC2(AD2), PC3(AD3), PC4(AD4/SDA), PC5(AD5/SCL)
#define chPC0  2
#define chPC1  3
#define chPC2  0
#define chPC3  1
// Note, missing channels 12/13 (AD6/AD7)

// PD0(0/RXI), PD1(0/TXD), PD2(2), PD3(3), PD4(4), PD5(5), PD6(6), PD7(7)
#define chPD2  9  
#define chPD3  8
#define chPD4  11
#define chPD5  10
#define chPD6  13
#define chPD7  12


// Serial/Error messages
#define SERIAL_WELCOME_MESSAGE        "OpenCT2 Imaging Array Controller"
#define SERIAL_WELCOME_MESSAGE2       "Commands: " 
#define SERIAL_WELCOME_MESSAGE3       "Set all digipots: A <1-255>"
#define SERIAL_WELCOME_MESSAGE4       "Set digipot on one channel: D <1-16> <1-255>"
#define SERIAL_WELCOME_MESSAGE5       "Integrate/measure: I <seconds>"
#define SERIAL_PARSE_ERROR            "UNRECOGNIZED INPUT"
#define SERIAL_SUCCESSFUL_COMMAND     "OK"
#define SERIAL_PROMPT                 "> "



/*
 * MCP23017 I/O Expander
 */ 
 
#define CS_INACTIVE  1
#define CS_ACTIVE    0
 
// Initialize the I/O expander
void setupIOExpander() {
  // MCP23017 I/O Expander  (for detector CS lines)
  IOExpander.begin();                        // Use default address (0)

  // Set all pins (which are chip selects) to outputs  
  for (int i=0; i<16; i++) {
    IOExpander.pinMode(i, OUTPUT);
  }  
  
  // Set all values to inactive
  for (int i=0; i<16; i++) {
    setCS(i, CS_INACTIVE);
  }  
  
}

// Set the chip select value on a given channel
void setCS(uint8_t channel, uint8_t value) {
  uint8_t channelMap[16];
  channelMap[0] = 7;
  channelMap[1] = 6;
  channelMap[2] = 5;
  channelMap[3] = 4;
  channelMap[4] = 3;
  channelMap[5] = 2;
  channelMap[6] = 1;
  channelMap[7] = 0;

  //## Note, below may be in reversed order
  /*
  channelMap[8] = 15;
  channelMap[9] = 14;
  channelMap[10] = 13;
  channelMap[11] = 12;
  channelMap[12] = 11;
  channelMap[13] = 10;
  channelMap[14] = 9;
  channelMap[15] = 8;
*/
  channelMap[8] = 8;
  channelMap[9] = 9;
  channelMap[10] = 10;
  channelMap[11] = 11;
  channelMap[12] = 12;
  channelMap[13] = 13;
  channelMap[14] = 14;
  channelMap[15] = 15;

  
  IOExpander.digitalWrite(channelMap[channel], value);
  
}

/*
 * Digipot
 */ 
void setupDigipot() {
  // MCP23008 I/O Expander (for SPI lines)
  IOExpanderSPI.begin(7);     // Address lines are all high (7)

  // Set all pins to inputs as default (most are unused)
  for (int i=0; i<16; i++) {
    IOExpanderSPI.pinMode(i, INPUT);
  }  

  // Set SPI lines to output
  IOExpanderSPI.pinMode(DIGIPOT_MOSI, OUTPUT);
  IOExpanderSPI.pinMode(DIGIPOT_SCLK, OUTPUT);   
}
 

void setDigipot(uint8_t channel, uint8_t value) {
  // Idle
  uint8_t val = value;
  IOExpanderSPI.digitalWrite(DIGIPOT_SCLK, 1);
  delay(2);
  setCS(channel, CS_ACTIVE);
  delay(2);  
  
  for (int i=0; i<8; i++) {
    IOExpanderSPI.digitalWrite(DIGIPOT_SCLK, 0);        // Inactive
    delay(1);
    
    // Data
    if ((val & 0x80) > 0) {
      IOExpanderSPI.digitalWrite(DIGIPOT_MOSI, 1);
      // Serial.print("1");
    } else {
      IOExpanderSPI.digitalWrite(DIGIPOT_MOSI, 0);
      // Serial.print("0");      
    }
    val = val << 1;
    delay(1);
    
    IOExpanderSPI.digitalWrite(DIGIPOT_SCLK, 1);      // Active
    delay(2);
      
  }
  
  IOExpanderSPI.digitalWrite(DIGIPOT_SCLK, 0);      // Active
  delay(2);
  
  setCS(channel, CS_INACTIVE);  
  // Serial.println("");
}
 
// Set all Digipot channels to a given value 
void setAllDigipotChannels(uint8_t value) {
  for (int i=0; i<MAX_CHANNELS; i++) {
    setDigipot(i, value); 
  }  
}
 
/*
 * Detector Channel Interrupt Initialization
 */
 
// Set the pins for all sensor channels to input
void setupSensorChannelPins() {
  pinMode(CH0, INPUT); 
  pinMode(CH1, INPUT);
  pinMode(CH2, INPUT);
  pinMode(CH3, INPUT);
  pinMode(CH4, INPUT);
  pinMode(CH5, INPUT);
  pinMode(CH6, INPUT);
  pinMode(CH7, INPUT);
  pinMode(CH8, INPUT);
  pinMode(CH9, INPUT);
  pinMode(CH10, INPUT);
  pinMode(CH11, INPUT);
  pinMode(CH12, INPUT);
  pinMode(CH13, INPUT);
  pinMode(CH14, INPUT);  
  pinMode(CH15, INPUT);    
}

// Enable pin change interrupt on all channels 
void enableSensorChannelInterrupts() {
  enablePCI(CH0);
  enablePCI(CH1);
  enablePCI(CH2);
  enablePCI(CH3);
  enablePCI(CH4);
  enablePCI(CH5);
  enablePCI(CH6);        
  enablePCI(CH7);
  enablePCI(CH8);
  enablePCI(CH9);
  enablePCI(CH10);
  enablePCI(CH11);
  enablePCI(CH12);
  enablePCI(CH13);
  enablePCI(CH14);  
  enablePCI(CH15);    
}

 
/*
 * Pin change interrupts
 */ 
void enablePCI(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));    // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin));                     // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin));                     // enable interrupt for the group
}
 
#define maskB0  B00000001
#define maskB1  B00000010
#define maskB2  B00000100
#define maskB3  B00001000
#define maskB4  B00010000
#define maskB5  B00100000
#define maskB6  B01000000
#define maskB7  B10000000

// ISR for PORT B (pins D8-D13)
uint8_t lastPORTB = 0;
ISR (PCINT0_vect) {
  uint8_t curPORTB = PINB;                                      // get pin states
  uint8_t diff = lastPORTB ^ curPORTB;                          // find differences between current and last states  
  
  if ((diff & maskB0) > 0)    countPin(chPB0, bitRead(curPORTB, 0));  
  if ((diff & maskB1) > 0)    countPin(chPB1, bitRead(curPORTB, 1));
  if ((diff & maskB2) > 0)    countPin(chPB2, bitRead(curPORTB, 2));
  if ((diff & maskB3) > 0)    countPin(chPB3, bitRead(curPORTB, 3));  
  if ((diff & maskB4) > 0)    countPin(chPB4, bitRead(curPORTB, 4));  
  if ((diff & maskB5) > 0)    countPin(chPB5, bitRead(curPORTB, 5));  

  lastPORTB = curPORTB;  
}

 
// ISR for PORT C (pins A0-A5)
uint8_t lastPORTC = 0;
ISR (PCINT1_vect) {
  uint8_t curPORTC = PINC;                                      // get pin states
  uint8_t diff = lastPORTC ^ curPORTC;                          // find differences between current and last states  
  lastPORTC = curPORTC;  
    
  if ((diff & maskB0) > 0)    countPin(chPC0, curPORTC & maskB0);
  if ((diff & maskB1) > 0)    countPin(chPC1, curPORTC & maskB1);
  if ((diff & maskB2) > 0)    countPin(chPC2, curPORTC & maskB2);
  if ((diff & maskB3) > 0)    countPin(chPC3, curPORTC & maskB3);
  
  
}  

// ISR for PORT D (pins D0-D7)
uint8_t lastPORTD = 0;
ISR (PCINT2_vect) {
  uint8_t curPORTD = PIND;                                      // get pin states
  uint8_t diff = lastPORTD ^ curPORTD;                          // find differences between current and last states  
  lastPORTD = curPORTD;
  
  if ((diff & maskB2) > 0)    countPin(chPD2, curPORTD & maskB2);
  if ((diff & maskB3) > 0)    countPin(chPD3, curPORTD & maskB3);
  if ((diff & maskB4) > 0)    countPin(chPD4, curPORTD & maskB4);
  if ((diff & maskB5) > 0)    countPin(chPD5, curPORTD & maskB5);
  if ((diff & maskB6) > 0)    countPin(chPD6, curPORTD & maskB6);
  if ((diff & maskB7) > 0)    countPin(chPD7, curPORTD & maskB7);  
}  
  
/*
 * Counting
 */ 

// Set all counts to zero 
void clearCounts() {
  for (int i=0; i<MAX_CHANNELS; i++) {
    channelCounts[i] = 0;
    channelLastTimes[i] = 0;
    channelStartTimes[i] = 0;
    
    for (int j=0; j<MAX_HISTBINS; j++) {
      channelHist[i][j] = 0;
    }
  }
}
  
// This is the main function that performs counting.  It should be called twice -- once when the pin initially becomes 
// active, and again when the pin becomes inactive.  It calculates both number of counts, and a histogram of pulse-width times. 
inline void countPin(uint8_t channel, uint8_t state) {
/*  
  Serial.print("CH");
  Serial.print(channel);  
  Serial.print(" ");
  Serial.print(state, BIN);
  Serial.println("");
*/  
  
  if (state > 0) {
      // Transitioning to active state
      channelStartTimes[channel] = micros();                       // Store start time of pulse (for determining pulse length)
  } else {
    // Transitioning to inactive state
      long unsigned int curTime = micros();
      long unsigned int deltaTime = curTime - channelLastTimes[channel];
      long unsigned int pulseWidthTime = curTime - channelStartTimes[channel];
      if (deltaTime > SENSOR_DEBOUNCETIME) {                      // Debounce
        if (channelCounts[channel] < MAX_COUNTS_PER_CHANNEL) {      // Prevent overflow errors
          channelCounts[channel] += 1;                              // Increment channel count
          uint8_t histBin = (pulseWidthTime >> 2)-3;                // Calculate histogram bin (NOTE: millis() has a resolution of 4uSec, and is always a multiple of four)
          if (histBin >= MAX_HISTBINS) histBin = MAX_HISTBINS-1;    // Last histogram bin is a catch-all
          channelHist[channel][histBin] += 1;                       // Increment histogram bin count
          channelLastTimes[channel] = micros();                     // Store start time of pulse (for debouncing)       
        }
      }
  }
} 
 
 
 
/*
 * Display
 */
void displayChannel(uint8_t channel) {
  Serial.print(channel);
  Serial.print("\t");
  Serial.print(channelCounts[channel]);
  Serial.println("");
} 
 
void displayChannelHist(uint8_t channel) {
  Serial.print(channel);
  Serial.print("\t");
  Serial.print(channelCounts[channel]);
  Serial.print("\t");
  
  for (int bin=0; bin<MAX_HISTBINS; bin++) {
    Serial.print(channelHist[channel][bin]);
    Serial.print("\t");
  }
  Serial.println("");
} 

void displayAllChannels() {
  for (int i=0; i<MAX_CHANNELS; i++) {
    displayChannel(i);
  } 
}

void displayAllChannelsHist() {
  for (int i=0; i<MAX_CHANNELS; i++) {
    displayChannelHist(i);
  } 
}

void displayCountsCondensed() {
  for (int i=0; i<MAX_CHANNELS; i++) {
    Serial.print(channelCounts[i]);
    Serial.print("\t");
  }   
  Serial.println("");
}


/*
 * High-level detection
 */ 
// Read data on all channels for a number of seconds, and display the results to the serial console
void readAllChannels(unsigned long integrationTimeSec, boolean withHist) {
  // Clear counts
  clearCounts();
  
  // Ensure that the integration time is specified in seconds, and not excessively large
  if (integrationTimeSec > 600) {
    Serial.println("ERROR: INTEGRATION TIME TOO LARGE"); 
  }
  
  // Collect data for integrationTimeSec seconds using interrupt service routines
  /*
  Serial.print ("Integrating for "); 
  Serial.print (integrationTimeSec);
  Serial.println (" seconds ");
  */
  delay(integrationTimeSec * 1000);
  
  // Report back detector values
  if (!withHist) {
    displayCountsCondensed();
  } else {
    displayAllChannelsHist();    
  }
  
}



/*
 * Serial command parsing
 */ 
 
// Parse serial command
// Parses commands of the form "<char> <space> <integer>"
// e.g. "I 10" to sample from the array for 10 seconds

int parseSerialCommand(String in) {
  // Check for well-formed string
  if (in.length() < 1) {
    Serial.println ("TOO SHORT");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }
  
  // Check for space(s)
  int spaceIdx = in.indexOf(" ");
  if (spaceIdx <= 0) {
    Serial.println ("NO SPACE");    
    Serial.println(SERIAL_PARSE_ERROR);
    return -1;
  }
  int secondSpaceIdx = in.indexOf(" ", spaceIdx+1);
  boolean multiArgument = false;
  if (secondSpaceIdx > 1) {
    multiArgument = true;
  }
  
  // Parse
  String command = in.substring(0, spaceIdx);
  String intArgument1 = "";
  if (!multiArgument) {
    intArgument1 = in.substring(spaceIdx + 1);
  } else {
    intArgument1 = in.substring(spaceIdx + 1, secondSpaceIdx);
  }
  String intArgument2 = "";
  if (multiArgument) {
    intArgument2 = in.substring(secondSpaceIdx + 1);
  }
  
  // Trim Whitespace
  command.trim();
  char commandChar = 0; 
  intArgument1.trim();
  int argument1 = intArgument1.toInt();
  argument1 = constrain(argument1, 0, 600);    // Clip argument to range (0, 600)
  intArgument2.trim();
  int argument2 = intArgument2.toInt();
  argument2 = constrain(argument2, 0, 255);    // Clip argument to range (0, 255)
  
  // Ensure command and argument are well-formed
  if (command.length() != 1) {
    Serial.println ("COMMAND NOT SINGLE CHARACTER");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  } else {
   commandChar = command[0]; 
  }
  if (intArgument1.length() <= 0) {
    Serial.println ("INT ARGUMENT MISSING");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }
  // If String.toInt() returns a zero, there was an error parsing the String into an integer
  if (argument1 == 0) {
    Serial.println ("INT ARGUMENT PARSE ERROR");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }    
  
  // Command and Argument are well formed
  switch(commandChar) {
    // Collect and report data on all channels for argument1 seconds
    case 'I':
      readAllChannels(argument1, false);
      break;
    
    // Set digipot value for all sensor channels
    case 'A':
      if ((argument1 <= 0) || (argument1 > 255)) {
        Serial.println("DIGIPOT VALUE MUST BE BETWEEN 1 AND 255");
        Serial.println(SERIAL_PARSE_ERROR);
        return -1;
      } 
      
      setAllDigipotChannels(argument1);
      Serial.print ("Set digipot values on all channels to ");
      Serial.println(argument1);
      break; 
      
    // Set digipot value for a particular sensor channel
    case 'D':
      //moveAxes(Z_AXIS, argument);
      if ((argument1 <= 0) && (argument1 > MAX_CHANNELS)) {
        Serial.println("UNRECOGNIZED CHANNEL");
        Serial.println(SERIAL_PARSE_ERROR);
        return -1;
      } 
      if ((argument2 <= 0) || (argument2 > 255)) {
        Serial.println("DIGIPOT VALUE MUST BE BETWEEN 1 AND 255");
        Serial.println(SERIAL_PARSE_ERROR);
        return -1;
      } 
      
      setDigipot(argument1 - 1, argument2);  
      Serial.print ("Set digipot on channel ");
      Serial.print (argument1);
      Serial.print (" to ");
      Serial.println(argument2);
      break; 
      
      
    default:
      Serial.println("UNRECOGNIZED COMMAND");
      Serial.println (SERIAL_PARSE_ERROR);
      return -1;
      break;       
  }
  
  /*
  Serial.print("COMMAND: ");
  Serial.println(command);
  Serial.print("ARGUMENT1: ");
  Serial.println(argument1);
  Serial.print("ARGUMENT2: ");
  Serial.println(argument2);
  */
  
  // Return success 
  Serial.println (SERIAL_SUCCESSFUL_COMMAND);
  return 1;
  
}
 
 
 
 
/*
 * Main
 */ 

// Initial parameters for data collection
uint8_t digipotValue = 170;
uint8_t digipotMinVal = 150;
//uint8_t digipotMaxVal = 170;
uint8_t digipotMaxVal = 190;
uint8_t digipotDelta = 0;

unsigned long integrationTime = 10000;
float secondCounter = 0.0f;

#define DIGIPOT_DEFAULTVAL    250

void setup() {
    
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println(SERIAL_WELCOME_MESSAGE);
  Serial.println(SERIAL_WELCOME_MESSAGE2);
  Serial.println(SERIAL_WELCOME_MESSAGE3);
  Serial.println(SERIAL_WELCOME_MESSAGE4);
  Serial.println(SERIAL_WELCOME_MESSAGE5);
 
  // Clear Counts
  clearCounts();
  
  // Setup pins
  setupDigipot();
  setupIOExpander();
  setupSensorChannelPins();  
  
  // Set Digipots to default value
  setAllDigipotChannels(DIGIPOT_DEFAULTVAL);
  
  // Enable sensor channel interrupts
  Serial.println ("Enabling interrupts... ");
  delay(500);
  enableSensorChannelInterrupts();
  
  Serial.println ("Initialization Complete... ");
  
  // Console
  Serial.println(SERIAL_SUCCESSFUL_COMMAND);
  Serial.print(SERIAL_PROMPT);
  
}
 

// Main loop / serial console
String serialBuffer;
void loop() {
  
  // Step 1: When available, Read Serial Data
  while (Serial.available() > 0) {
    // Read one character
    char oneChar = Serial.read();
    
    // If that character is a newline, then the user has completed the command. 
    if (oneChar == '\n') {
      // Parse the string
      Serial.println("");
      // Serial.print("INPUT: ");
      // Serial.println(serialBuffer);
      
      parseSerialCommand(serialBuffer);      
      serialBuffer = "";      // Clear buffer      
      Serial.print(SERIAL_PROMPT);
      
      
    } else {
      // Add character to string
      serialBuffer += oneChar;  
      Serial.print(oneChar);            // Echo character
    }
      
  }
  
}

