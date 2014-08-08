/*-----------------------------------------------------------------------------------------------
/* door bell support: Tchibo / Heidemann HX Pocket (70283)
-----------------------------------------------------------------------------------------------*/

#include "doorbell.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

extern unsigned int timings[];

/*
 * TCM234759 Stuff
 * Message: 100100111100 11100001
 *          Address      ?
 */

bool receiveProtocolTCM(unsigned int changeCount) {
#define TCM_SYNC          47670
#define TCM_ONE           750
#define TCM_ZERO          1400
#define TCM_GLITCH        100

#define TCM_MESSAGELENGTH 20

  if (changeCount < TCM_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings[0] < TCM_SYNC - TCM_GLITCH) || (timings[0] > TCM_SYNC + TCM_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[TCM_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, TCM_MESSAGELENGTH * 2, TCM_ZERO - TCM_GLITCH, TCM_ZERO + TCM_GLITCH, TCM_ONE - TCM_GLITCH, TCM_ONE + TCM_GLITCH) == false) {
      return false;
  }
#endif

  String rawcode;
  rawcode = RawMessage(timings, TCM_MESSAGELENGTH, TCM_ZERO - TCM_GLITCH, TCM_ZERO + TCM_GLITCH, TCM_ONE - TCM_GLITCH, TCM_ONE + TCM_GLITCH);
  if (rawcode == "") {
      return false;
  }

  message = "M";
  message += rawcode;
  available = true;
  return true;
}

void TCM_CMDs(String cmd) {
  // Set Tchibo Repetition
  if (cmd.startsWith("dr")) {
    char msg[3];
    cmd.substring(3).toCharArray(msg,3);
    TCMrepetition = atoi(msg);
    Serial.println(cmd);
  }  
  // Switch Tchibo door bell Devices
  else if (cmd.startsWith("ds")) {
    // tc11011100111011100001#
    digitalWrite(PIN_LED,HIGH);
    char msg[21];
    cmd.substring(2).toCharArray(msg,21);
    // sendTCM(msg);
    sendStd(msg, TCMrepetition, 548, 1404, 508, 760, 1160, 47612);
    digitalWrite(PIN_LED,LOW);
    Serial.println(cmd);
  }
  else if (cmd.startsWith("dp")) {
    String cPrint = "TCMParams: ";
    cPrint += " ";
    cPrint += String(HXrepetition);
    Serial.println(cPrint);
  }
}

/*
 * Heidemann HX Pocket (70283)
 * May work also with other Heidemann HX series door bells
 */

bool receiveProtocolHX(unsigned int changeCount) {
#define HX_SYNC   5030
#define HX_ONE    710
#define HX_ZERO   350
#define HX_GLITCH  40

#define HX_MESSAGELENGTH 12

  if (changeCount != 25) {
    return false;
  }
  if ((timings[0] < HX_SYNC - HX_GLITCH) || (timings[0] > HX_SYNC + HX_GLITCH)) {
    return false;
  }

  bool bitmessage[HX_MESSAGELENGTH];
  
#ifdef DEBUG
  if (GetBitStream(timings, bitmessage, HX_MESSAGELENGTH * 2, HX_ZERO - HX_GLITCH, HX_ZERO + HX_GLITCH, HX_ONE - HX_GLITCH, HX_ONE + HX_GLITCH) == false) {
      return false;
  }
#endif

  String rawcode;
  rawcode = RawMessage(timings, HX_MESSAGELENGTH, HX_ZERO - HX_GLITCH, HX_ZERO + HX_GLITCH, HX_ONE - HX_GLITCH, HX_ONE + HX_GLITCH);
  if (rawcode == "") {
      return false;
  }

  message = "H";
  message += rawcode;
  available = true;
  return true;
}

void HeideTX_CMDs(String cmd) {
  if (cmd.startsWith("hr")) {
    char msg[3];
    cmd.substring(3).toCharArray(msg,3);
    HXrepetition = atoi(msg);
    Serial.println(cmd);
  }  
  // Switch Intertechno Devices
  else if (cmd.startsWith("hs")) {
    // hx110111110001#
    digitalWrite(PIN_LED,HIGH);
    char msg[15];
    cmd.substring(2).toCharArray(msg,15);
    // sendHX(msg);
    sendStd(msg, HXrepetition, 270, 300, 600, 720, 260, 5000);
    digitalWrite(PIN_LED,LOW);
    Serial.println(cmd);
  }
  else if (cmd.startsWith("hp")) {
    String cPrint = "TXParams: ";
    cPrint += " ";
    cPrint += String(HXrepetition);
    Serial.println(cPrint);
  }
}

void sendStd(char* StateMessage, byte Repetition, int Intro, int Nlow, int Nhigh, int Hlow, int Hhigh, unsigned long Footer) {
  unsigned int pos = 0;
  unsigned int milli = 0;
  unsigned int micro = 0;
  
  if (Footer > 16000) {
    milli = Footer / 1000;
    micro = Footer - (milli * 1000);
  } else {
    micro = Footer;
  }
  
  for (int i = 0; i < Repetition; i++) {
    pos = 0;
    disableReceive();
    digitalWrite(PIN_SEND, HIGH);
    delayMicroseconds(Intro); 
    enableReceive();
    while (StateMessage[pos] != '\0') {
      switch(StateMessage[pos]) {
      case '0':
        disableReceive();
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(Nlow); 
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(Nhigh); 
        enableReceive();
        break;
      case '1':
        disableReceive();
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(Hlow); 
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(Hhigh); 
        enableReceive();
        break;
      }
      pos++;
    }
    disableReceive();
    digitalWrite(PIN_SEND, LOW);
    delayMicroseconds(micro);
    delay(milli);
    enableReceive();
  }
}

