/*
 * EOF preprocessor bug prevent
*/
/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mdorenka + jowiemann + sidey, mick6300
// see http://forum.fhem.de/index.php/topic,17196.0.html
//
// Test sketch for Auriol temperature sensor

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduinoTest for Auriol"
#define PROGVERS               "0.0"

#if defined(__AVR_ATmega32U4__)          //on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
#define PIN_RECEIVE            3
#else
#define PIN_RECEIVE            2
#endif

#define PIN_LED                13
#define PIN_SEND               11

#define DEBUG           // Compile sketch witdh Debug informations
#ifdef DEBUG
#define BAUDRATE               115200
#else
#define BAUDRATE               9600
#endif

/*
 * Weather sensors
 */
#define MAX_CHANGES            90
unsigned int timings[MAX_CHANGES];

String cmdstring;
volatile bool available = false;
String message = "";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  // wait for the serial monitor to open.
#ifdef DEBUG
    delay(3000);
    Serial.println(" -------------------------------------- ");
    Serial.print("    ");
    Serial.print(PROGNAME);
    Serial.print(" ");
    Serial.println(PROGVERS);
    Serial.println(" -------------------------------------- ");
#endif

  enableReceive();
  pinMode(PIN_RECEIVE,INPUT);
  pinMode(PIN_SEND,OUTPUT);

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
  static unsigned long lastTime;

  duration = micros() - lastTime;
  
  decoders(duration);

  lastTime += duration;
}

/*
 * call decoders when valid timings sequence available
 */
void decoders(unsigned int duration) {
#define STARTBIT_TIME   5000
#define STARTBIT_OFFSET 200

  static unsigned int changeCount;
  static unsigned int repeatCount;
  bool rc = false;

  if (duration > STARTBIT_TIME && duration > timings[0] - STARTBIT_OFFSET && duration < timings[0] + STARTBIT_OFFSET) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      // update uptime
      uptime(millis(), false);
      if (rc == false) {
        rc = receiveProtocolAureol(changeCount);
      }
      repeatCount = 0;
    }
    changeCount = 0;
  } 
  else if (duration > STARTBIT_TIME) {
    changeCount = 0;
  }

  if (changeCount >= MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  timings[changeCount++] = duration;
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
    case '#':
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
  else if (cmd.equals("t")) {
    uptime(millis(), true);
  }
  else if (cmd.equals("XQ")) {
    disableReceive();
    Serial.flush();
    Serial.end();
  }
  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V R q"));
  }
  cmdstring = "";
}

void uptime(unsigned long timepassed, bool Print)
{
  static unsigned long time_in_secs=0;
  static unsigned long temps=0;
  unsigned long secs=0;

  secs = timepassed/1000;
  
  if (secs < temps) {
     time_in_secs += secs;
  } else {
     time_in_secs = secs;
  }

  temps = timepassed/1000;
  
  if (Print) {
    //Display results
    String cPrint = "00000000";
    cPrint += String(time_in_secs,HEX);
    int StringStart = cPrint.length()-8;
    cPrint = cPrint.substring(StringStart);
    cPrint.toUpperCase();
    Serial.println(cPrint);
  }
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

/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality
-----------------------------------------------------------------------------------------------*/

/*
 * Aureol
 */
bool receiveProtocolAureol(unsigned int changeCount) {
#define Aureol_SYNC 9200
#define Aureol_ONE 3900
#define Aureol_ZERO 1950
#define Aureol_GLITCH 150
#define Aureol_MESSAGELENGTH 32

  if (changeCount != (Aureol_MESSAGELENGTH * 2) + 1) {
//    Serial.println(changeCount);
    return false;
  }

  if ((timings[0] < Aureol_SYNC - Aureol_GLITCH) || (timings[0] > Aureol_SYNC + Aureol_GLITCH)) {
    return false;
  }

#ifdef DEBUG
  bool bitmessage[Aureol_MESSAGELENGTH];
  if (GetBitStream(timings, bitmessage, Aureol_MESSAGELENGTH * 2, Aureol_ZERO - Aureol_GLITCH, Aureol_ZERO + Aureol_GLITCH, Aureol_ONE - Aureol_GLITCH, Aureol_ONE + Aureol_GLITCH) == false) {
    Serial.println("Err: BitStream");
    return false;
  }
#endif

  String rawcode;
  rawcode = RawMessage(timings, Aureol_MESSAGELENGTH, Aureol_ZERO - Aureol_GLITCH, Aureol_ZERO + Aureol_GLITCH, Aureol_ONE - Aureol_GLITCH, Aureol_ONE + Aureol_GLITCH);

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

#ifdef DEBUG
bool GetBitStream(unsigned int timings[] ,bool bitmessage[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One) {

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
#endif

String RawMessage(unsigned int timings[], byte BitCount, unsigned int L_Zero, unsigned int R_Zero, unsigned int L_One, unsigned int R_One) {

  byte i=0;
  byte j=0;
  byte code = 0;
  String hexText;
  
  for (i = 0; i < BitCount * 2; i = i + 2)
  {
    j = i + 2;
    if ((timings[j] > L_Zero) && (timings[j] < R_Zero))    {
      code = code << 1;
    }
    else if ((timings[j] > L_One) && (timings[j] < R_One)) {
      code <<= 1;
      code |= 1;
    }
    else {
      Serial.println("Err RawIntern: ");
      return "";
    }
    // Byte then chnage to hex
    if ((j % 8) == 0) {
      hexText += String(code,HEX);
      code = 0;
    }
  }
  
  // maybe some bits left
  if ((j % 8) != 0) {
    hexText += String(code,HEX);
  }

  return hexText;
}

unsigned long hexToDec(String hexString) {
  
  unsigned long decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}

