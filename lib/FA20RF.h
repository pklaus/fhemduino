/*-----------------------------------------------------------------------------------------------
/* Smoke dector FA20RF
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _FA20RF_h
  #define _FA20RF_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

void disableReceive();
void enableReceive();

bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
unsigned long hexToDec(String hexString);

/*
 * FA20RF Receiver
 */
static byte FArepetition = 10;

void FA20RF(unsigned int duration);
/*
 * FA20RF Decoder
 */
void receiveProtocolFA20RF(unsigned int changeCount);

void sendFA20RF(char* StateMessage);

void FA20RF_CMDs(String cmd);


