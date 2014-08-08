/*-----------------------------------------------------------------------------------------------
/* door bell support: Tchibo / Heidemann HX Pocket (70283)
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _doorbell_h
  #define _doorbell_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

#include "sketch.h"

/*-----------------------------------------------------------------------------------------------
/* Devices with sending / receiving functionality
-----------------------------------------------------------------------------------------------*/
void disableReceive();
void enableReceive();

bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One);
unsigned long hexToDec(String hexString);

/*
 * TCM234759 Stuff
 * Message: 100100111100 11100001
 *          Address      ?
 */
static byte TCMrepetition = 19;

bool receiveProtocolTCM(unsigned int changeCount);

void TCM_CMDs(String cmd);

/*
 * Heidemann HX Pocket (70283)
 * May work also with other Heidemann HX series door bells
 */
static  unsigned int HXrepetition = 19;

bool receiveProtocolHX(unsigned int changeCount);

void HeideTX_CMDs(String cmd);

/*
 * Transmission functions
 * 
 */

void sendStd(char* StateMessage, byte Repetition, int Intro, int Nlow, int Nhigh, int Hlow, int Hhigh, unsigned long Footer);



