/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality
-----------------------------------------------------------------------------------------------*/

#include "temphum.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

extern unsigned int timings[];
extern unsigned int timings2500[];

#ifdef COMP_KW9010
/*
 * KW9010
 */
bool receiveProtocolKW9010(unsigned int changeCount) {
#define KW9010_SYNC 9000
#define KW9010_ONE 4000
#define KW9010_ZERO 2000
#define KW9010_GLITCH 200
#define KW9010_MESSAGELENGTH 36

  if (changeCount < KW9010_MESSAGELENGTH * 2) return false;

  if ((timings[0] < KW9010_SYNC - KW9010_GLITCH) || (timings[0] > KW9010_SYNC + KW9010_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[KW9010_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, KW9010_MESSAGELENGTH * 2, KW9010_ZERO - KW9010_GLITCH, KW9010_ZERO + KW9010_GLITCH, KW9010_ONE - KW9010_GLITCH, KW9010_ONE + KW9010_GLITCH) == false) {
      return false;
  }
#endif

  String rawcode;
  rawcode = RawMessage(timings, KW9010_MESSAGELENGTH, KW9010_ZERO - KW9010_GLITCH, KW9010_ZERO + KW9010_GLITCH, KW9010_ONE - KW9010_GLITCH, KW9010_ONE + KW9010_GLITCH);

  if (rawcode == "") {
    return false;
  }

  String crcbitmessage = hex2bin(rawcode);
  
  // check Data integrity
  byte checksum = 0;
  checksum += (byte)(crcbitmessage[35] << 3 | crcbitmessage[34] << 2 | crcbitmessage[33] << 1 | crcbitmessage[32]);
  checksum &= 0xF;
  
  byte calculatedChecksum = 0;
  for (int i = 0 ; i <= 7 ; i++) {
    calculatedChecksum += (byte)(crcbitmessage[i*4 + 3] << 3 | crcbitmessage[i*4 + 2] << 2 | crcbitmessage[i*4 + 1] << 1 | crcbitmessage[i*4]);
  }
  calculatedChecksum &= 0xF;

  if (calculatedChecksum == checksum) {
    message = "W01";
    message += rawcode;
    available = true;
    return true;
  }

  return false;

}
#endif

#ifdef COMP_EUROCHRON
/*
 * EUROCHRON
 */
bool receiveProtocolEuroChron(unsigned int changeCount) {
#define Euro_SYNC   8050
#define Euro_ONE    2020
#define Euro_ZERO   1010
#define Euro_GLITCH  100
#define Euro_MESSAGELENGTH 36

  if (changeCount < Euro_MESSAGELENGTH * 2) {
    return false;
  }

  if ((timings[0] < Euro_SYNC - Euro_GLITCH) && (timings[0] > Euro_SYNC + Euro_GLITCH)) {
    return false;
  }
  
#ifdef DEBUG
  bool bitmessage[Euro_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, Euro_MESSAGELENGTH * 2, Euro_ZERO - Euro_GLITCH, Euro_ZERO + Euro_GLITCH, Euro_ONE - Euro_GLITCH, Euro_ONE + Euro_GLITCH) == false) {
      return false;
  }
#endif
  String rawcode;
  rawcode = RawMessage(timings, Euro_MESSAGELENGTH, Euro_ZERO - Euro_GLITCH, Euro_ZERO + Euro_GLITCH, Euro_ONE - Euro_GLITCH, Euro_ONE + Euro_GLITCH);

  if (rawcode == "") {
      return false;
  }

  message = "W02";
  message += rawcode;
  available = true;
  return true;

}
#endif

#ifdef COMP_NC_WS
/*
 * NC_WS / PEARL NC7159, LogiLink W0002
 */
bool receiveProtocolNC_WS(unsigned int changeCount) {
#define NCWS_SYNC   9250
#define NCWS_ONE    3900
#define NCWS_ZERO   1950
#define NCWS_GLITCH 100
#define NCWS_MESSAGELENGTH 36

  if (changeCount < NCWS_MESSAGELENGTH * 2) {
    return false;
  }
  
  if ((timings[0] < NCWS_SYNC - NCWS_GLITCH) || (timings[0] > NCWS_SYNC + NCWS_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[NCWS_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, NCWS_MESSAGELENGTH * 2, NCWS_ZERO - NCWS_GLITCH, NCWS_ZERO + NCWS_GLITCH, NCWS_ONE - NCWS_GLITCH, NCWS_ONE + NCWS_GLITCH) == false) {
      return false;
  }
#endif
  String rawcode;
  rawcode = RawMessage(timings, NCWS_MESSAGELENGTH, NCWS_ZERO - NCWS_GLITCH, NCWS_ZERO + NCWS_GLITCH, NCWS_ONE - NCWS_GLITCH, NCWS_ONE + NCWS_GLITCH);
  if (rawcode == "") {
      return false;
  }

  // First 4 bits always 0101 == 5
  if (rawcode[0] != '5') {
    return false;
  }

  message = "W03";
  message += rawcode;
  available = true;
  return true;

}
#endif

#ifdef COMP_LIFETEC
/*
 * LIFETEC
 */
bool receiveProtocolLIFETEC(unsigned int changeCount) {
#define LIFE_SYNC   8640
#define LIFE_ONE    4084
#define LIFE_ZERO   2016
#define LIFE_GLITCH  460
#define LIFE_MESSAGELENGTH 24

  if (changeCount < LIFE_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings[0] < LIFE_SYNC - LIFE_GLITCH) || (timings[0] > LIFE_SYNC + LIFE_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[TX_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, LIFE_MESSAGELENGTH * 2, LIFE_ZERO - LIFE_GLITCH, LIFE_ZERO + LIFE_GLITCH, LIFE_ONE - LIFE_GLITCH, LIFE_ONE + LIFE_GLITCH) == false) {
      return false;
  }
#endif
  String rawcode;
  rawcode = RawMessage(timings, LIFE_MESSAGELENGTH, LIFE_ZERO - LIFE_GLITCH, LIFE_ZERO + LIFE_GLITCH, LIFE_ONE - LIFE_GLITCH, LIFE_ONE + LIFE_GLITCH);
  if (rawcode == "") {
      return false;
  }

  message = "W04";
  message += rawcode;
  message += "___"; // Fill up to 12 signs
  available = true;
  return true;

}
#endif

#ifdef COMP_TX70DTH
/*
 * TX70DTH
 */
bool receiveProtocolTX70DTH(unsigned int changeCount) {
#define TX_SYNC   4000
#define TX_ONE    2030
#define TX_ZERO   1020
#define TX_GLITCH  250
#define TX_MESSAGELENGTH 36

  if (changeCount < TX_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings2500[0] < TX_SYNC - TX_GLITCH) || (timings2500[0] > TX_SYNC + TX_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[TX_MESSAGELENGTH];
  if (GetBitStream(timings2500, bitmessage, TX_MESSAGELENGTH * 2, TX_ZERO - TX_GLITCH, TX_ZERO + TX_GLITCH, TX_ONE - TX_GLITCH, TX_ONE + TX_GLITCH) == false) {
      return false;
  }
#endif
  String rawcode;
  rawcode = RawMessage(timings2500, TX_MESSAGELENGTH, TX_ZERO - TX_GLITCH, TX_ZERO + TX_GLITCH, TX_ONE - TX_GLITCH, TX_ONE + TX_GLITCH);
  if (rawcode == "") {
      return false;
  }

  message = "W05";
  message += rawcode;
  available = true;
  return true;
}
#endif

#ifdef COMP_AURIOL
/*
 * AURIOL (Lidl Version: 09/2013)
 */
bool receiveProtocolAURIOL(unsigned int changeCount) {
#define AURIOL_SYNC 9200
#define AURIOL_ONE 3900
#define AURIOL_ZERO 1950
#define AURIOL_GLITCH 150
#define AURIOL_MESSAGELENGTH 32

  if (changeCount != (AURIOL_MESSAGELENGTH * 2) + 1) {
    return false;
  }

  if ((timings[0] < AURIOL_SYNC - AURIOL_GLITCH) || (timings[0] > AURIOL_SYNC + AURIOL_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[AURIOL_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, AURIOL_MESSAGELENGTH * 2, AURIOL_ZERO - AURIOL_GLITCH, AURIOL_ZERO + AURIOL_GLITCH, AURIOL_ONE - AURIOL_GLITCH, AURIOL_ONE + AURIOL_GLITCH) == false) {
    Serial.println("Err: BitStream");
    return false;
  }
#endif

  String rawcode;
  rawcode = RawMessage(timings, AURIOL_MESSAGELENGTH, AURIOL_ZERO - AURIOL_GLITCH, AURIOL_ZERO + AURIOL_GLITCH, AURIOL_ONE - AURIOL_GLITCH, AURIOL_ONE + AURIOL_GLITCH);

  if (rawcode == "") {
    return false;
  }

  // check Data integrity
  message = "W06";
  message += rawcode;
  message += "_"; // Fill up to 12 signs
  available = true;
  return true;

}
#endif
