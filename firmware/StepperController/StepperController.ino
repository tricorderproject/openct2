// OpenCT2 Stepper Controller/Parser
// Peter Jansen, October/2015
// 
// Usage: Accepts serial commands of the form <char> <int_argument>, 
// Where: <char> defines the axis of motion (either R or Z)
// <int_argument> defines how many steps to move, between -255 and 255. 
//
// Requires: Adafruit Motor Shield Library

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <Adafruit_PWMServoDriver.h>

// Pin Definitions
#define SWITCH_ZHOMELIMIT   12
#define SWITCH_ZTOPLIMIT    13

// Constants
#define Z_AXIS  1
#define R_AXIS  2

// Step modes in the stepper driver.  Valid modes are: SINGLE, DOUBLE, INTERLEAVE, MICROSTEP
#define Z_STEP_MODE   DOUBLE
#define R_STEP_MODE   INTERLEAVE

// Scaling factor for Z axis
#define ZSCALE  10

// Error messages
#define SERIAL_WELCOME_MESSAGE        "OpenCT2 Motion Controller"
#define SERIAL_WELCOME_MESSAGE2       "Format: <char> <int>, where <char> = R,Z and <int> = -255 to 255"
#define SERIAL_PARSE_ERROR            "UNRECOGNIZED INPUT"
#define SERIAL_SUCCESSFUL_COMMAND     "OK"
#define SERIAL_PROMPT                 "> "


// Motor Shield
// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *zAxisMotor = AFMS.getStepper(200, 2);
Adafruit_StepperMotor *rAxisMotor = AFMS.getStepper(200, 1);



// Moves the Z or R axes
void moveAxes(int whichAxis, int distance) {
  
  switch(whichAxis) {
    case Z_AXIS:
      // The Z axis has a fine pitched lead screw, so multiply the distance by zScale so that we don't have to 
      // send large numbers over the serial console    
      if (distance > 0) {
        for (int i=0; i<distance; i++) {
          if (limitTop() == false) {
            zAxisMotor->step(1 * ZSCALE, FORWARD, Z_STEP_MODE);
          }
        }
      } else {
        for (int i=0; i<-distance; i++) {
          if (limitHome() == false) {
            zAxisMotor->step(1 * ZSCALE, BACKWARD, Z_STEP_MODE);   
          }
        }    
      }
      
      // Turn off Z stepper 
      zAxisMotor->release();
      break;
      
    case R_AXIS:
      // Rotational Axis
      if (distance > 0) {
        rAxisMotor->step(distance, FORWARD, R_STEP_MODE); 
      } else {
        rAxisMotor->step(-distance, BACKWARD, R_STEP_MODE);       
      }

      // Turn off Z stepper 
      rAxisMotor->release();      
      break;
      
    default:
      // Error: Axis not recognized
      break;
  }
    
}


// Parse serial command
// Parses commands of the form "<char> <space> <integer>"
// e.g. "R -100" to move the rotational axis backwards 100 steps
// "Z 50" to move the Z axis upwards 50 steps. 
int parseSerialCommand(String in) {
  // Check for well-formed string
  if (in.length() < 1) {
    Serial.println ("TOO SHORT");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }
  
  // Check for space
  int spaceIdx = in.indexOf(" ");
  if (spaceIdx <= 0) {
    Serial.println ("NO SPACE");    
    Serial.println(SERIAL_PARSE_ERROR);
    return -1;
  }
  
  // Parse
  String command = in.substring(0, spaceIdx);
  String intArgument = in.substring(spaceIdx + 1);
  
  // Trim Whitespace
  command.trim();
  char commandChar = 0; 
  intArgument.trim();
  int argument = intArgument.toInt();
  argument = constrain(argument, -255, 255);    // Clip argument to -255 to 255 range
  
  // Ensure command and argument are well-formed
  if (command.length() != 1) {
    Serial.println ("COMMAND NOT SINGLE CHARACTER");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  } else {
   commandChar = command[0]; 
  }
  if (intArgument.length() <= 0) {
    Serial.println ("INT ARGUMENT MISSING");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }
  // If String.toInt() returns a zero, there was an error parsing the String into an integer
  if (argument == 0) {
    Serial.println ("INT ARGUMENT PARSE ERROR");
    Serial.println (SERIAL_PARSE_ERROR);
    return -1;
  }    
  
  // Command and Argument are well formed
  switch(commandChar) {
    case 'H':
      homeZ();
      break;
      
    case 'R':
      moveAxes(R_AXIS, argument);
      break;
     
    case 'Z':
      moveAxes(Z_AXIS, argument);   
      break; 
      
    default:
      Serial.println("UNRECOGNIZED COMMAND");
      Serial.println (SERIAL_PARSE_ERROR);
      return -1;
      break;       
  }
  
  Serial.print("COMMAND: ");
  Serial.println(command);
  Serial.print("ARGUMENT: ");
  Serial.println(argument);
  
  // Return success 
  Serial.println (SERIAL_SUCCESSFUL_COMMAND);
  return 1;
  
}


// Home the Z axis
void homeZ() {
  int maxDist = 10000;
  for (int i=0; i<maxDist; i++) {
    if (limitHome() == false) {
      zAxisMotor->step(1 * ZSCALE, BACKWARD, Z_STEP_MODE);   
    }    
  }
}


// Limit Switches
bool limitHome() {
  if (digitalRead(SWITCH_ZHOMELIMIT) == LOW) {
    return true;
  }
  return false;
}

bool limitTop() {
  if (digitalRead(SWITCH_ZTOPLIMIT) == LOW) {
    return true;
  }
  return false;
}

// Initialization
void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println(SERIAL_WELCOME_MESSAGE);
  Serial.println(SERIAL_WELCOME_MESSAGE2);

  // Initialize stepper driver
  AFMS.begin(4000);  // OR with a different frequency, say 1KHz
 
  zAxisMotor->setSpeed(5000);  // Set the Z axis as fast as it will go
  rAxisMotor->setSpeed(10);    // 10 rpm   

  // Initialize limit switch pins
  pinMode(SWITCH_ZHOMELIMIT, INPUT_PULLUP);
  pinMode(SWITCH_ZTOPLIMIT, INPUT_PULLUP);
  
  // Console
  Serial.println(SERIAL_SUCCESSFUL_COMMAND);
  Serial.print(SERIAL_PROMPT);

}


// Main loop
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
