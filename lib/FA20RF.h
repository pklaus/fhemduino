/*-----------------------------------------------------------------------------------------------
/* Smoke dector FA20RF
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile FA20RF witdh Debug informations

#ifndef _FA20RF_h
  #define _FA20RF_h
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
 * FA20RF Receiver
 */
static unsigned int FArepetition = 10;

void FA20RF(unsigned int duration);
/*
 * FA20RF Decoder
 */
void receiveProtocolFA20RF(unsigned int changeCount);

void sendFA20RF(char* StateMessage);

