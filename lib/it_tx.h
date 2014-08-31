/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _IT_TX_h
  #define _IT_TX_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

void IT_TX(unsigned int duration);

void receiveProtocolIT_TX(unsigned int changeCount);

