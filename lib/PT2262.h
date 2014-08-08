/*-----------------------------------------------------------------------------------------------
/* Devices with sending / receiving functionality => PT2262
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile PT2262 witdh Debug informations

#ifndef _PT2262_h
  #define _PT2262_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#if defined(__AVR_ATmega32U4__)          //  
#define PIN_SEND               10        // on some 32U Devices, there is no PIN 11, so we use 10 here. 
#else 
#define PIN_SEND               11 
#endif 

void disableReceive();
void enableReceive();

bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
unsigned long hexToDec(String hexString);

/*
 * PT2262 Stuff
 */
#define RECEIVETOLERANCE       60
static byte ITrepetition = 6;
static byte ITreceivetolerance = 60;
static unsigned int ITbaseduration = 350;

bool receiveProtocolPT2262(unsigned int changeCount);

void sendPT2262(char* triStateMessage);

void PT2262_transmit(int nHighPulses, int nLowPulses);

