/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality
-----------------------------------------------------------------------------------------------*/

#ifndef _TEMP_HUM_h
  #define _TEMP_HUM_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
unsigned long hexToDec(String hexString);

bool receiveProtocolKW9010(unsigned int changeCount);

bool receiveProtocolEuroChron(unsigned int changeCount);

bool receiveProtocolNC_WS(unsigned int changeCount);

bool receiveProtocolLIFETEC(unsigned int changeCount);

bool receiveProtocolTX70DTH(unsigned int changeCount);

bool receiveProtocolAURIOL(unsigned int changeCount);

String hex2bin(String hexaDecimal);

