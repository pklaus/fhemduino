/*-----------------------------------------------------------------------------------------------
/* Devices with sending / receiving functionality => PT2262
-----------------------------------------------------------------------------------------------*/

#include "PT2262.h"

extern String cmdstring;
extern volatile bool available;
extern String message;

extern unsigned int timings[];

bool receiveProtocolPT2262(unsigned int changeCount) {

  if (changeCount != 49) {
    return false;
  }

  message = "IR";
  unsigned long code = 0;
  unsigned long delay = timings[0] / 31;
  unsigned long delayTolerance = delay * ITreceivetolerance * 0.01; 

  for (int i = 1; i < changeCount; i=i+2) {
    if (timings[i] > delay-delayTolerance && timings[i] < delay+delayTolerance && timings[i+1] > delay*3-delayTolerance && timings[i+1] < delay*3+delayTolerance) {
      code = code << 1;
    }
    else if (timings[i] > delay*3-delayTolerance && timings[i] < delay*3+delayTolerance && timings[i+1] > delay-delayTolerance && timings[i+1] < delay+delayTolerance)  { 
      code += 1;
      code = code << 1;
    }
    else {
      code = 0;
      return false;
    }
  }
  code = code >> 1;

#ifdef DEBUG
  Serial.print(changeCount);
  Serial.print(" ");
  Serial.print(code,BIN);
  Serial.print(" ");
  Serial.print(delay);
  Serial.print(" ");
  Serial.println(delayTolerance);
#endif

  message += code;
  message += "_";
  message += delay;
  available = true;
  return true;
}

void sendPT2262(char* triStateMessage) {
  //unsigned int BaseDur = 350; // Um ggf. die Basiszeit einstellen zu k�nnen
  for (int i = 0; i < ITrepetition; i++) {
    unsigned int pos = 0;
    PT2262_transmit(1,31);
    while (triStateMessage[pos] != '\0') {
      switch(triStateMessage[pos]) {
      case '0':
        PT2262_transmit(1,3);
        PT2262_transmit(1,3);
        break;
      case 'F':
        PT2262_transmit(1,3);
        PT2262_transmit(3,1);
        break;
      case '1':
        PT2262_transmit(3,1);
        PT2262_transmit(3,1);
        break;
      }
      pos++;
    }
  }
}

void PT2262_transmit(int nHighPulses, int nLowPulses) {
  disableReceive();
  digitalWrite(PIN_SEND, HIGH);
  delayMicroseconds(ITbaseduration * nHighPulses);
  digitalWrite(PIN_SEND, LOW);
  delayMicroseconds(ITbaseduration * nLowPulses);
  enableReceive();
}

