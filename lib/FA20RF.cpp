/*-----------------------------------------------------------------------------------------------
/* Smoke dector FA20RF
-----------------------------------------------------------------------------------------------*/

#include "FA20RF.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

extern unsigned int timings[];


/*
 * Receiver
 */
#define FA20_MAX_CHANGES 60
unsigned int timingsFA20[FA20_MAX_CHANGES];      //  FA20RF

void FA20RF(unsigned int duration) {
#define L_STARTBIT_TIME         8020
#define H_STARTBIT_TIME         8120
#define L_STOPBIT_TIME          10000
#define H_STOPBIT_TIME          20000 // Stop bit is something geater 10.000 us, may be not graeter then 20.000.

  static unsigned int changeCount;

  if (duration > L_STARTBIT_TIME && duration < H_STARTBIT_TIME) {
    changeCount = 0;
    timingsFA20[0] = duration;
  } 
  else if ((duration > L_STOPBIT_TIME && duration < H_STOPBIT_TIME) && ( timingsFA20[0] > L_STARTBIT_TIME && timingsFA20[0] < H_STARTBIT_TIME)) {
    timingsFA20[changeCount] = duration;
    receiveProtocolFA20RF(changeCount);
    changeCount = 0;
  }

  if (changeCount >= FA20_MAX_CHANGES) {
    changeCount = 0;
  }
  timingsFA20[changeCount++] = duration;
}

/*
 * FA20RF Decoder
 */
void receiveProtocolFA20RF(unsigned int changeCount) {
#define FA20RF_SYNC   8060
#define FA20RF_SYNC2  960
#define FA20RF_ONE    2740
#define FA20RF_ZERO   1450
#define FA20RF_GLITCH  70
#define FA20RF_MESSAGELENGTH 24

  if (changeCount < (FA20RF_MESSAGELENGTH * 2)) {
    return;
  }
  
  if ((timingsFA20[0] < FA20RF_SYNC - FA20RF_GLITCH) || (timingsFA20[0] > FA20RF_SYNC + FA20RF_GLITCH)) {
    return;
  }

  if ((timingsFA20[1] < FA20RF_SYNC2 - FA20RF_GLITCH) || (timingsFA20[1] > FA20RF_SYNC2 + FA20RF_GLITCH)) {
    return;
  }

  byte i;
  unsigned long code = 0;

  for (i = 1; i < (FA20RF_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timingsFA20[i + 2] > FA20RF_ZERO - FA20RF_GLITCH) && (timingsFA20[i + 2] < FA20RF_ZERO + FA20RF_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsFA20[i + 2] > FA20RF_ONE - FA20RF_GLITCH) && (timingsFA20[i + 2] < FA20RF_ONE + FA20RF_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
#ifdef DEBUG
      Serial.print("timingsFA20[");
      Serial.print(i + 2);
      Serial.print("]: ");
      Serial.println(timingsFA20[i + 2]);
      Serial.print("timingsFA20[51]: ");
      Serial.println(timingsFA20[51]);
#endif
      return;
    }
  }

#ifdef DEBUG
  Serial.println(code,BIN);
#endif

  char tmp[5];
  message = "F";
  message += String(code,HEX);

  sprintf(tmp, "%05u", timingsFA20[i+2]);
  message += "-";
  message += tmp;

  available = true;
  return;
}

void sendFA20RF(char* StateMessage) {
  unsigned int pos = 0;

  for (int i = 0; i < FArepetition; i++) {
    delay(1);
    pos = 0;
    disableReceive();
    digitalWrite(PIN_SEND, HIGH);
    delayMicroseconds(8040);
    digitalWrite(PIN_SEND, LOW);
    delayMicroseconds(920);
    enableReceive();
    while (StateMessage[pos] != '\0') {
      switch(StateMessage[pos]) {
      case '0':
        disableReceive();
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(740);
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(1440);
        enableReceive();
        break;
      case '1':
        disableReceive();
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(740);
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(2740);
        enableReceive();
        break;
      }
      pos++;
    }
    disableReceive();
    digitalWrite(PIN_SEND, HIGH);
    delayMicroseconds(750);
    digitalWrite(PIN_SEND, LOW);
    delayMicroseconds(12000);
    digitalWrite(PIN_SEND, HIGH);
    delayMicroseconds(35);
    digitalWrite(PIN_SEND, LOW);
    enableReceive();
  }
}

void FA20RF_CMDs(String cmd) {
  // Set FA20RF Repetition
  if (cmd.startsWith("fr"))
  {
    char msg[3];
    cmd.substring(3).toCharArray(msg,3);
    FArepetition = atoi(msg);
    Serial.println(cmd);
  }  
  // Switch FA20RF Devices
  else if (cmd.startsWith("fs"))
  {
    digitalWrite(PIN_LED,HIGH);
    char msg[30];
    cmd.substring(2).toCharArray(msg,30);
    sendFA20RF(msg);
    digitalWrite(PIN_LED,LOW);
    Serial.println(msg);
  }
  else if (cmd.startsWith("fp")) {
    String cPrint = "FAParams: ";
    cPrint += " ";
    cPrint += String(FArepetition);
    Serial.println(cPrint);  
  }
}
