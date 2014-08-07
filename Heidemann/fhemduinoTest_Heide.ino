/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mdorenka + jowiemann
// see http://forum.fhem.de/index.php/topic,17196.0.html
//
//
// Test sketch for Heidemann door bells

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduinoTest for Heidemann door bell"
#define PROGVERS               "0.0"

#define MAX_CHANGES            75
#define BAUDRATE               115200
#define RECEIVETOLERANCE       60
#define PIN_LED                13
#define PIN_SEND               11
#if defined(__AVR_ATmega32U4__)          //on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
#define PIN_RECEIVE            3
#else
#define PIN_RECEIVE            2
#endif

bool    DEBUG                  = true;      // set to true to see debug messages

unsigned int timings[MAX_CHANGES];
String cmdstring;
volatile bool available = false;
String message = "";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  enableReceive();
  pinMode(PIN_RECEIVE,INPUT);
  pinMode(PIN_SEND,OUTPUT);
  Serial.println("----------------------START------------------");
}

void loop() {
  // put your main code here, to run repeatedly: 
  if (messageAvailable()) {
    Serial.println(message);
    resetAvailable();
  }
//serialEvent does not work on ATmega32U4 devices like the Leonardo, so we do the handling ourselves
#if defined(__AVR_ATmega32U4__)
  if (Serial.available()) {
    serialEvent();
  }
#endif
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
  static unsigned int aduration;
  static unsigned int fduration;
  static unsigned int changeCount;
  static unsigned long lastTime;
  static unsigned int repeatCount;

  long time = micros();
  duration = time - lastTime;
  
  if (duration > 2500 && duration > timings[0] - 100 && duration < timings[0] + 100) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      if (receiveProtocolHX(changeCount) == false) {
        // failed
      }
      repeatCount = 0;
    }
    changeCount = 0;
  } 
  else if (duration > 2500) {
    changeCount = 0;
  }

  if (changeCount >= MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  timings[changeCount++] = duration;
  lastTime = time;
  aduration = fduration;
  fduration = duration;
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
    case '#':
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
    Serial.println(F("V " PROGVERS " FHEMduino - compiled at " __DATE__ " " __TIME__));
  }
  // Print free Memory
  else if (cmd.equals("R")) {
    Serial.print(F("R"));
    Serial.println(freeRam());
  }
  // call TCM doorbell
  else if (cmd.startsWith("hx"))
  {
    digitalWrite(PIN_LED,HIGH);
    char msg[15];
    cmd.substring(2).toCharArray(msg,15);
    sendHX(msg);
    digitalWrite(PIN_LED,LOW);
    Serial.println(msg);
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
  
  if (GetBitStream(bitmessage, HX_MESSAGELENGTH * 2, HX_ZERO - HX_GLITCH, HX_ZERO + HX_GLITCH, HX_ONE - HX_GLITCH, HX_ONE + HX_GLITCH) == false) {
      return false;
  }

  byte i;
  unsigned long code = 0;
  message = "HX";
  char tmp[3];

  // Bit 1..4 => Address of door bell
  // DIP-Switch
  // 4321 0=OFF / 1 = ON
  // 0000 => 0000
  // 1000 => 0001
  // 0100 => 0010
  // 0010 => 0100
  // 0001 => 1000
  // 1100 => 0011
  // 1111 => 1111
  for (i = 0; i < 4; i++)  if (bitmessage[i]) code +=  1 << (3 - i);
  sprintf(tmp, "%02d", code);
  message += tmp;
  // message += "-";

  code = 0;
  for (i = 4; i < 7; i++)  if (bitmessage[i]) code +=  1 << (6 - i);
  sprintf(tmp, "%01d", code);
  message += tmp;
  // message += "-";

  // Bit 8..12 => Sound of door bell
  // 10011 => 1. 2xDing-Dong
  // 10101 => 2. Telefonklingeln
  // 11001 => 3. Zirkusmusik
  // 11101 => 4. Banjo on my knee
  // 11110 => 5. Morgen kommt der Weihnachtsmann
  // 10110 => 6. It’s a small world
  // 10010 => 7. Hundebellen
  // 10001 => 8. Westminster
  
  code = 0;
  for (i = 7; i < 12; i++)  if (bitmessage[i]) code +=  1 << (11 - i);
  sprintf(tmp, "%02d", code);
  message += tmp;
  // message += "-";

  // Bit 13 => May be batterie state of door bell sender
  code = 0;
  if ((timings[25] > 280 - HX_GLITCH) && (timings[25] < 280 + HX_GLITCH))    {
    code += 1;
    code = code << 1;
  }
  else if ((timings[25] > 580 - HX_GLITCH) && (timings[25] < 580 + HX_GLITCH)) {
    code = code << 1;
  }
  code = code >> 1;

  message += code;
 
  available = true;
  return true;
}

bool GetBitStream(bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One) {

  byte i=0;
  byte j=0;
  
  for (i = 0; i < BitCount; i = i + 2)
  {
    j = i + 2;
    if ((timings[j] > L_Zero) && (timings[j] < R_Zero))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings[j] > L_One) && (timings[j] < R_One)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }

  Serial.print("Bit-Stream: ");
  for (i = 0; i < (BitCount / 2); i++) {
    Serial.print(bitmessage[i]);
  }
  Serial.println();

  return true;
}

void sendHX(char* triStateMessage) {
  unsigned int pos = 0;

  // hx110111110001#
  for (int i = 0; i < 19; i++) {
    pos = 0;
    disableReceive();
    digitalWrite(PIN_SEND, HIGH);
    delayMicroseconds(270);
    enableReceive();
    while (triStateMessage[pos] != '\0') {
      switch(triStateMessage[pos]) {
      case '0':
        disableReceive();
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(360);
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(600);
        enableReceive();
        break;
      case '1':
        disableReceive();
        digitalWrite(PIN_SEND, LOW);
        delayMicroseconds(720);
        digitalWrite(PIN_SEND, HIGH);
        delayMicroseconds(260);
        enableReceive();
        break;
      }
      pos++;
    }
    disableReceive();
    digitalWrite(PIN_SEND, LOW);
    delayMicroseconds(5000);
    enableReceive();
  }
}
