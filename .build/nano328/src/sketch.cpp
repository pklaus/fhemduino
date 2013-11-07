#include <Arduino.h>
void setup();
void loop();
void serialEvent();
void HandleCommand(String cmd);
void ISRHandler();
void EZ6Interrupt();
int freeRam ();
#line 1 "src/sketch.ino"
#include "RCSwitch.h"
#include <stdio.h>
#include <stdarg.h>

#define SERIAL_BITRATE 9600
String cmdstring;
RCSwitch mySwitch = RCSwitch();
boolean isTransmitting = false;

// Defines
#define DataBits0 4                                       // Number of data0 bits to expect
#define DataBits1 32                                      // Number of data1 bits to expect
#define allDataBits 36                                    // Number of data sum 0+1 bits to expect
// isrFlags bit numbers
#define F_HAVE_DATA 1                                     // 0=Nothing in read buffer, 1=Data in read buffer
#define F_GOOD_DATA 2                                     // 0=Unverified data, 1=Verified (2 consecutive matching reads)
#define F_CARRY_BIT 3                                     // Bit used to carry over bit shift from one long to the other
#define F_STATE 7                                         // 0=Sync mode, 1=Data mode

// Constants
const unsigned long sync_MIN = 4300;                      // Minimum Sync time in micro seconds
const unsigned long sync_MAX = 4700;

const unsigned long bit1_MIN = 2300;
const unsigned long bit1_MAX = 2700;

const unsigned long bit0_MIN = 1330;
const unsigned long bit0_MAX = 1730;

const unsigned long glitch_Length = 300;                  // Anything below this value is a glitch and will be ignored.

// Interrupt variables
unsigned long fall_Time = 0;                              // Placeholder for microsecond time when last falling edge occured.
unsigned long rise_Time = 0;                              // Placeholder for microsecond time when last rising edge occured.
byte bit_Count = 0;                                       // Bit counter for received bits.
unsigned long build_Buffer[] = {
  0,0};                     // Placeholder last data packet being received.
volatile unsigned long read_Buffer[] = {
  0,0};             // Placeholder last full data packet read.
volatile byte isrFlags = 0;                               // Various flag bits

void setup()
{
  Serial.begin(SERIAL_BITRATE);
  pinMode(2,INPUT);
  pinMode(13,OUTPUT);
  attachInterrupt(0,ISRHandler,CHANGE);
}

void loop()
{
  if (mySwitch.available()) {

    int value = mySwitch.getReceivedValue();

    if (value != 0) {
      Serial.print(F("I"));
      Serial.println(mySwitch.getReceivedValue());
    }

    mySwitch.resetAvailable();
  }


  if (bitRead(isrFlags,F_GOOD_DATA) == 1) 
  {
	detachInterrupt(0);
	
    unsigned long myData0 = read_Buffer[0];                             // Read the data spread over 2x 32 variables
    unsigned long myData1 = read_Buffer[1];
    bitClear(isrFlags,F_HAVE_DATA);                       // Flag we have read the data

    byte BatteryInfo = (myData1 >> 26) & 0x3;             // Get Battery
    byte RandomID = (myData1 >> 28) | (myData0 << 4);     // Get Random ID
    byte Channel = ((myData1 >> 24) & 0x3) + 1;           // Get Channel

    byte ML = (myData1 >> 12) & 0xF0;                     // Get MMMM
    byte H = (myData1 >> 12) & 0xF;                       // Get LLLL
    ML = ML | H;                                          // OR MMMM & LLLL nibbles together
    H = (myData1 >> 20) & 0xF;                            // Get HHHH
    byte HH = 0;
    if((myData1 >> 23) & 0x1 == 1)                        //23 bit
    {
      HH = 0xF;
    }
    int Temperature = (H << 8) | (HH << 12) | ML;           // Combine HHHH MMMMLLLL

    H = (myData1 >> 0) & 0xF0;                          // Get HHHH
    ML = (myData1 >> 0) & 0xF;                            // Get LLLL
    byte Humidity =  ML | H;                              // OR HHHH & LLLL nibbles together

	
	// Output the Result on Serial
	char tmp[12];
	sprintf(tmp,"E%02X%01d%01d%+04d%03d",RandomID,Channel,BatteryInfo,Temperature,Humidity);
	Serial.println(tmp);
	
    
	attachInterrupt(0,ISRHandler,CHANGE);
  }
  delay(100);


}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();

    switch(inChar)
    {
    case '\n':
    case '\r':
    case '\0':
      HandleCommand(cmdstring);
      break;
    default:
      cmdstring = cmdstring + inChar;
    }
  }
}

void HandleCommand(String cmd)
{
  // Version Information
  if (cmd.equals("V") || cmd.equals("version"))
  {
    Serial.println(F("V 0.3 RFduino - compiled at " __DATE__ " " __TIME__));
  }

  // Switch Intertechno Devices
  else if (cmd.startsWith("is"))
  {
    digitalWrite(13,HIGH);
    detachInterrupt(0);

    mySwitch.enableTransmit(11);
    mySwitch.setProtocol(1);
    // Send it 8 times to make sure it gets transmitted
    mySwitch.setRepeatTransmit(8);
	char msg[13];
	cmd.substring(2).toCharArray(msg,13);
    mySwitch.sendTriState(msg);
    // Disable Transmitter
    mySwitch.disableTransmit();

    attachInterrupt(0,ISRHandler,CHANGE);
    digitalWrite(13,LOW);
	Serial.println(cmd);
  }
  
  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V"));
  }
  

  cmdstring = "";
}

void ISRHandler()
{
    // Call RFSwitch Interrupt Handler
    mySwitch.handleInterrupt();
  
    // Call EZ6 Interrupt Handler
    EZ6Interrupt();
}

void EZ6Interrupt()
{
  unsigned long currentMicros = micros();

  // Reset if an Overflow of micros() has occured since last rising edge
  if (currentMicros < rise_Time)
  {
    rise_Time = currentMicros;
    fall_Time = currentMicros;

    build_Buffer[0] = 0;
    build_Buffer[1]= 0;
    read_Buffer[0] = 0;
    read_Buffer[1] = 0;
    isrFlags = 0;

    return;
  }

  if (digitalRead(2) == LOW)
  {
    // Falling edge
    if (currentMicros > (rise_Time + glitch_Length))
    {
      // Not a glitch
      unsigned long pulseTime = currentMicros - fall_Time;                        // Subtract last falling edge to get pulse time.
      if (bitRead(build_Buffer[1],31) == 1)
      {
        bitSet(isrFlags, F_CARRY_BIT);
      }
      else
      {
        bitClear(isrFlags, F_CARRY_BIT);
      }

      if (bitRead(isrFlags, F_STATE) == 1)
      {
        // Looking for Data
        if ((pulseTime > bit0_MIN) && (pulseTime < bit0_MAX))
        {
          // 0 bit
          build_Buffer[1] = build_Buffer[1] << 1;
          build_Buffer[0] = build_Buffer[0] << 1;
          if (bitRead(isrFlags,F_CARRY_BIT) == 1)
          {
            bitSet(build_Buffer[0],0);
          }
          bit_Count++;
        }
        else if ((pulseTime > bit1_MIN) && (pulseTime < bit1_MAX))
        {
          // 1 bit
          build_Buffer[1] = build_Buffer[1] << 1;
          bitSet(build_Buffer[1],0);
          build_Buffer[0] = build_Buffer[0] << 1;
          if (bitRead(isrFlags,F_CARRY_BIT) == 1)
          {
            bitSet(build_Buffer[0],0);
          }
          bit_Count++;
        }
        else
        {
          // Not a 0 or 1 bit so restart data build and check if it's a sync?
          bit_Count = 0;
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bitClear(isrFlags, F_GOOD_DATA);                // Signal data reads dont' match
          bitClear(isrFlags, F_STATE);                    // Set looking for Sync mode
          if ((pulseTime > sync_MIN) && (pulseTime < sync_MAX))
          {
            // Sync length okay
            bitSet(isrFlags, F_STATE);                    // Set data mode
          }
        }
        if (bit_Count >= allDataBits)
        {
          // All bits arrived
          bitClear(isrFlags, F_GOOD_DATA);                // Assume data reads don't match
          if (build_Buffer[0] == read_Buffer[0])
          {
            if (build_Buffer[1] == read_Buffer[1])
            {
              bitSet(isrFlags, F_GOOD_DATA);              // Set data reads match
            }
          }
          read_Buffer[0] = build_Buffer[0];
          read_Buffer[1] = build_Buffer[1];
          bitSet(isrFlags, F_HAVE_DATA);                  // Set data available
          bitClear(isrFlags, F_STATE);                    // Set looking for Sync mode
          digitalWrite(13,HIGH); // Used for debugging
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bit_Count = 0;
        }
      }
      else
      {
        // Looking for sync
        if ((pulseTime > sync_MIN) && (pulseTime < sync_MAX))
        {
          // Sync length okay
          build_Buffer[0] = 0;
          build_Buffer[1] = 0;
          bit_Count = 0;
          bitSet(isrFlags, F_STATE);                      // Set data mode
          digitalWrite(13,LOW); // Used for debugging
        }
      }
      fall_Time = currentMicros;                               // Store fall time
    }
  }
  else
  {
    // Rising edge
    if (currentMicros > (fall_Time + glitch_Length))
    {
      // Not a glitch
      rise_Time = currentMicros;                                   // Store rise time
    }
  }
}

// Get free RAM of UC
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
