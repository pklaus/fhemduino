#define MAX_CHANGES 75
#define BAUDRATE 9600
#define RECEIVETOLERANCE 60

unsigned int timings[MAX_CHANGES];
String cmdstring;
volatile bool available = false;
String message = "";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  enableReceive();
  pinMode(2,INPUT);
  pinMode(11,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly: 
  if (messageAvailable()) {
    Serial.println(message);
    resetAvailable();
  }
}
/*
 * Interrupt System
 */

void enableReceive() {
  attachInterrupt(0,handleInterrupt,CHANGE);
}

void disableReceive() {
  detachInterrupt(0);
}

void handleInterrupt() {
  static unsigned int duration;
  static unsigned int changeCount;
  static unsigned long lastTime;
  static unsigned int repeatCount;

  long time = micros();
  duration = time - lastTime;

  if (duration > 5000 && duration > timings[0] - 200 && duration < timings[0] + 200) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      if (receiveProtocolKW9010(changeCount) == false) {
        if (receiveProtocolPT2262(changeCount) == false) {
          // failed;
        }
      }
      repeatCount = 0;
    }
    changeCount = 0;
  } 
  else if (duration > 5000) {
    changeCount = 0;
  }

  if (changeCount >= MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  timings[changeCount++] = duration;
  lastTime = time;  
}


/*
 * Serial Command Handling
 */
void serialEvent()
{
  while (Serial.available())
  {

    char inChar = (char)Serial.read();
    switch(inChar)
    {
    case '\n':
    case '\r':
    case '\0':
      HandleCommand(cmdstring);
      break;
    default:
      cmdstring = cmdstring + inChar;
    }
  }
}

void HandleCommand(String cmd)
{
  // Version Information
  if (cmd.equals("V"))
  {
    Serial.println(F("V 1.0b1 FHEMduino - compiled at " __DATE__ " " __TIME__));
  }


  // Print free Memory
  else if (cmd.equals("R")) {
    Serial.print(F("R"));
    Serial.println(freeRam());
  }


  // Switch Intertechno Devices
  else if (cmd.startsWith("is"))
  {
    digitalWrite(13,HIGH);


    char msg[13];
    cmd.substring(2).toCharArray(msg,13);
    sendPT2262(msg);
    digitalWrite(13,LOW);
    Serial.println(cmd);
  }
  else if (cmd.equals("XQ")) {
    disableReceive();
    Serial.flush();
    Serial.end();
  }


  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V is R q"));
  }

  cmdstring = "";
}



// Get free RAM of UC
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

/*
 * Message Handling
 */
bool messageAvailable() {
  return (available && (message.length() > 0));
}

void resetAvailable() {
  available = false;
  message = "";
}

/*
 * KW9010
 */

bool receiveProtocolKW9010(unsigned int changeCount) {

#define KW9010_SYNC 9000
#define KW9010_ONE 4000
#define KW9010_ZERO 2000
#define KW9010_GLITCH 200
#define KW9010_MESSAGELENGTH 36

  if (changeCount < KW9010_MESSAGELENGTH * 2) {
    return false;
  }

  bool bitmessage[KW9010_MESSAGELENGTH + 1];
  int bitcount = 0;

  if ((timings[0] < KW9010_SYNC - KW9010_GLITCH) || (timings[0] > KW9010_SYNC + KW9010_GLITCH)) {
    return false;
  }
  //Serial.println(changeCount);
  for (int i = 2; i < changeCount; i=i+2) {
    if ((timings[i] > KW9010_ZERO - KW9010_GLITCH) && (timings[i] < KW9010_ZERO + KW9010_GLITCH)) {
      // its a zero
      bitmessage[bitcount] = false;
      bitcount++;
    }
    else if ((timings[i] > KW9010_ONE - KW9010_GLITCH) && (timings[i] < KW9010_ONE + KW9010_GLITCH)) {
      // its a one
      bitmessage[bitcount] = true;
      bitcount++;
    }
    else {
      return false;
    }
  }

  // Sensor ID & Channel
  byte id = bitmessage[7] | bitmessage[6] << 1 | bitmessage[5] << 2 | bitmessage[4] << 3 | bitmessage[3] << 4 | bitmessage[2] << 5 | bitmessage[1] << 6 | bitmessage[0] << 7;

  // (Propably) Battery State
  bool battery = bitmessage[8];

  // Trend
  byte trend = bitmessage[9] << 1 | bitmessage[10];

  // Trigger
  bool forcedSend = bitmessage[11];

  // Temperature & Humidity
  int temperature = ((bitmessage[23] << 11 | bitmessage[22] << 10 | bitmessage[21] << 9 | bitmessage[20] << 8 | bitmessage[19] << 7 | bitmessage[18] << 6 | bitmessage[17] << 5 | bitmessage[16] << 4 | bitmessage[15] << 3 | bitmessage[14] << 2 | bitmessage[13] << 1 | bitmessage[12]) << 4 ) >> 4;
  byte humidity = (bitmessage[31] << 7 | bitmessage[30] << 6 | bitmessage[29] << 5 | bitmessage[28] << 4 | bitmessage[27] << 3 | bitmessage[26] << 2 | bitmessage[25] << 1 | bitmessage[24]) - 156;

  // check Data integrity
  byte checksum = (bitmessage[35] << 3 | bitmessage[34] << 2 | bitmessage[33] << 1 | bitmessage[32]);
  byte calculatedChecksum = 0;

  for (int i = 0 ; i <= 7 ; i++) {
    calculatedChecksum += (byte)(bitmessage[i*4 + 3] <<3 | bitmessage[i*4 + 2] << 2 | bitmessage[i*4 + 1] << 1 | bitmessage[i*4]);
  }
  calculatedChecksum &= 0xF;

  if (calculatedChecksum == checksum) {
    if (temperature > -500 && temperature < 700) {
      if (humidity > 0 && humidity < 100) {
        char tmp[11];
        sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
        //Serial.println(tmp);
        message = tmp;
        available = true;
        return true;
      }
    }
  }
  return false;
}



/*
 * PT2262 Stuff
 */
bool receiveProtocolPT2262(unsigned int changeCount) {
  message = "IR";
  if (changeCount != 49) {
    return false;
  }
  unsigned long code = 0;
  unsigned long delay = timings[0] / 31;
  unsigned long delayTolerance = delay * RECEIVETOLERANCE * 0.01; 

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
      i = changeCount;
      return false;
    }
  }
  code = code >> 1;
  message += code;
  available = true;
  return true;
}

void sendPT2262(char* triStateMessage) {
  for (int i = 0; i < 3; i++) {
    unsigned int pos = 0;
    while (triStateMessage[pos] != '\0') {
      switch(triStateMessage[pos]) {
      case '0':
        PT2262_sendT0();
        break;
      case 'F':
        PT2262_sendTF();
        break;
      case '1':
        PT2262_sendT1();
        break;
      }
      pos++;
    }
    PT2262_sendSync();    
  }
}

void PT2262_sendT0() {
  PT2262_transmit(1,3);
  PT2262_transmit(1,3);
}
void PT2262_sendT1() {
  PT2262_transmit(3,1);
  PT2262_transmit(3,1);
}

void PT2262_sendTF() {
  PT2262_transmit(1,3);
  PT2262_transmit(3,1);
}

void PT2262_sendSync() {
  PT2262_transmit(1,31);
}

void PT2262_transmit(int nHighPulses, int nLowPulses) {
  disableReceive();
  digitalWrite(11, HIGH);
  delayMicroseconds(350 * nHighPulses);
  digitalWrite(11, LOW);
  delayMicroseconds(350 * nLowPulses);
  enableReceive();
}