/*-----------------------------------------------------------------------------------------------
/* Helper functions
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _HELPER_h
  #define _HELPER_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

void uptime(unsigned long timepassed, bool Print);

int freeRam ();

bool messageAvailable();

void resetAvailable();

bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);

String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);

unsigned long hexToDec(String hexString);

String hex2bin(String hexaDecimal);


