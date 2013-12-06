#include "RCSwitch.h"
#include <stdio.h>
#include <stdarg.h>

#define SERIAL_BITRATE 9600


#define SYNC 9500
#define ONE 4500
#define ZERO 2500
#define GLITCH 200

#define MESSAGELENGTH 36

unsigned long LastPulseTime = 0;

volatile bool SyncReceived = false;
volatile unsigned long bitcount = 0;
volatile bool message[MESSAGELENGTH + 1];


String cmdstring;
RCSwitch mySwitch = RCSwitch();
bool isTransmitting = false;




void setup()
{
  Serial.begin(SERIAL_BITRATE);
  pinMode(2,INPUT);
  pinMode(13,OUTPUT);
  attachInterrupt(0,ISRHandler,FALLING);
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

  if (SyncReceived) {
    // Sensor ID & Channel
    uint8_t id = message[7] | message[6] << 1 | message[5] << 2 | message[4] << 3 | message[3] << 4 | message[2] << 5 | message[1] << 6 | message[0] << 7;

    // (Propably) Battery State
    bool battery = message[8];

    // Trend
    uint8_t trend = message[9] << 1 | message[10];

    // Trigger
    bool forcedSend = message[11];

    // Temperature & Humidity
    int temperature = ((message[23] << 11 | message[22] << 10 | message[21] << 9 | message[20] << 8 | message[19] << 7 | message[18] << 6 | message[17] << 5 | message[16] << 4 | message[15] << 3 | message[14] << 2 | message[13] << 1 | message[12]) << 4 ) >> 4;
    uint8_t humidity = (message[31] << 7 | message[30] << 6 | message[29] << 5 | message[28] << 4 | message[27] << 3 | message[26] << 2 | message[25] << 1 | message[24]) - 156;
    
    // check Data integrity
    uint8_t checksum = (message[35] << 3 | message[34] << 2 | message[33] << 1 | message[32]);
    uint8_t calculatedChecksum = 0;
    for (int i = 0 ; i <= 7 ; i++) {
      calculatedChecksum += (byte)(message[i*4 + 3] <<3 | message[i*4 + 2] << 2 | message[i*4 + 1] << 1 | message[i*4]);
    }
    calculatedChecksum &= 0xF;

    if (calculatedChecksum == checksum) {
      char tmp[11];
      sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
      Serial.println(tmp);
    }

    SyncReceived = false;
    bitcount = 0;
  }
//delay(100);
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
  if (cmd.equals("V"))
  {
    Serial.println(F("V 0.4 RFduino - compiled at " __DATE__ " " __TIME__));
  }


  // Print free Memory
  else if (cmd.equals("R")) {
      Serial.print(F("R"));
      Serial.println(freeRam());
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

    attachInterrupt(0,ISRHandler,FALLING);
    digitalWrite(13,LOW);
    Serial.println(cmd);
  }
  

  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V is R"));
  }
  
  cmdstring = "";
}

void ISRHandler()
{
  // Call RFSwitch Interrupt Handler
  // mySwitch.handleInterrupt();

  // Call KW9010 Interrupt
  KW9010ISR();
}

void KW9010ISR() {
  unsigned long currentMicros = micros();
  
  if (LastPulseTime > currentMicros) {
    LastPulseTime = currentMicros;
    return;
  }

  if (bitcount > 36) {
    bitcount = 0;
  }
  unsigned long duration = currentMicros - LastPulseTime;
  LastPulseTime = currentMicros;
  if ((duration > ZERO - GLITCH) && (duration < SYNC + GLITCH)) {
    if ((duration > SYNC - GLITCH) && (duration < SYNC + GLITCH)) {
      if (bitcount == MESSAGELENGTH) {
          SyncReceived = true;
      }
      bitcount = 0;
    }
    if (!SyncReceived) {
      if ((duration > ZERO - GLITCH) && (duration < ZERO + GLITCH)) {
        // its a zero
        message[bitcount] = false;
        bitcount++;
      }
      else if ((duration > ONE - GLITCH) && (duration < ONE + GLITCH)) {
        // its a one
        message[bitcount] = true;
        bitcount++;
      }
    }
  }
}

// Get free RAM of UC
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
