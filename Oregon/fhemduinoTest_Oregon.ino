/*
 * EOF preprocessor bug prevent
*/
/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mdorenka + jowiemann + sidey, mick6300
// see http://forum.fhem.de/index.php/topic,17196.0.html
//
// Test sketch for Oregon

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduinoTest for Oregon"
#define PROGVERS               "0.0"

#if defined(__AVR_ATmega32U4__)          //on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
#define PIN_RECEIVE            3
#else
#define PIN_RECEIVE            2
#endif

#define PIN_LED                13

#if defined(__AVR_ATmega32U4__)          //  
#define PIN_SEND               10        // on some 32U Devices, there is no PIN 11, so we use 10 here. 
#else 
#define PIN_SEND               11 
#endif 

#define DEBUG           // Compile sketch witdh Debug informations
#ifdef DEBUG
#define BAUDRATE               115200
#else
#define BAUDRATE               9600
#endif

#define COMP_OSV2       // Compile sketch with OSV2 Support
#define COMP_Cresta     // Compile sketch with Cresta Support (currently not implemented, just for future use)
#define USE_OREGON_41   // Use oregon_41 Module which is already included in fhem. If not defined, the 14_fhemduino_oregon module will be used.

// Future enhancement
//#define COMP_OSV3     // Compile sketch with OSV3 Support (currently not implemented, just for future use)
//#define COMP_Kaku     // Compile sketch with Kaku  Support (currently not implemented, just for future use)
//#define COMP_HEZ      // Compile sketch with Homeeasy Support (currently not implemented, just for future use)
//#define COMP_XRF      // Compile sketch with XTF Support (currently not implemented, just for future use)

/*
 * Modified code to fit info fhemduino - Sidey
 * Oregon V2 decoder modfied - Olivier Lebrun
 * Oregon V2 decoder added - Dominique Pierre
 * New code to decode OOK signals from weather sensors, etc.
 * 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
 *
*/

#include "decoders.h";
#ifdef COMP_OSV2     // Compile sketch with OSV3 Support (currently not implemented, just for future use)
OregonDecoderV2 orscV2;
#endif

#ifdef COMP_Cresta     // Compile sketch with Cresta Support (currently not implemened, just for future use)
CrestaDecoder cres;
#endif

#ifdef COMP_Kaku     // Compile sketch with Kaku  Support (currently not implemented, just for future use)
KakuDecoder kaku;
#endif

#ifdef COMP_HEZ     // Compile sketch with Homeeasy Support (currently not implemented, just for future use)
HezDecoder hez;
#endif

#ifdef COMP_XRF
XrfDecoder xrf;
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
  enableReceive();
  pinMode(PIN_RECEIVE,INPUT);
  pinMode(PIN_SEND,OUTPUT);

#ifdef DEBUG
    delay(3000);
    Serial.println(" -------------------------------------- ");
    Serial.print("    ");
    Serial.print(PROGNAME);
    Serial.print(" ");
    Serial.println(PROGVERS);
    Serial.print(" "); 
    Serial.print("Free Ram: "); 
    Serial.println(freeRam()); 
    Serial.println(" -------------------------------------- ");
#endif

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

#ifdef COMP_OSV2
  COMP_OSV2_HANDLER (duration);
#endif

#ifdef COMP_Cresta
  COMP_Cresta_HANDLER (duration);
#endif

  lastTime += duration;
}

/*
 * call Oregon decoder when valid timings sequence available
 */
#ifdef COMP_OSV2
void COMP_OSV2_HANDLER (unsigned int duration) {
  if (orscV2.nextPulse(duration) )
  {
    uint8_t len;
    const byte* data = orscV2.getData(len);

    char tmp[36]="";
    char len_field[2]="";
    uint8_t tmp_len = 0;
    //strcat(tmp, "OSV2:");
    //tmp_len = 5;
#ifdef DEBUG
    Serial.print("HEXStream");
#endif

    for (byte i = 0; i < len; ++i) {
#ifdef DEBUG
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
        Serial.print(",");
#endif
      tmp_len += snprintf(tmp + tmp_len, 36, "%02X", data[i]);
    }

#ifdef DEBUG
    Serial.println(" ");
    Serial.println("Length:");
#endif


#ifdef USE_OREGON_41
    message=(String(len*8, HEX));
    message.concat(tmp);
//    message.concat("\n");
//    Dirty hack, just to test the 41_Oregon Module
//    Serial.println(" ");
//    Serial.print(len*8, HEX);
//    Serial.println(tmp);
#else
   message.concat("OSV2:");
   message.concat(tmp);
#endif
   orscV2.resetDecoder();

   available = true;
  }
}
#endif

#ifdef COMP_Cresta
void COMP_Cresta_HANDLER (unsigned int duration) {
  if (cres.nextPulse(duration))
  {
    byte len;
    const byte* data = orscV2.getData(len) + 5;
    char tmp[36]="";
    int tmp_len = 0;
    strcat(tmp, "CRESTA:");
    tmp_len = 7;

#ifdef DEBUG
      Serial.print("HEXStream");
#endif

    for (byte i = 0; i < len; ++i) {
#ifdef DEBUG
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
        Serial.print(",");
#endif
      tmp_len += snprintf(tmp + tmp_len, 36, "%02X", data[i]);
    }
    
#ifdef DEBUG
      Serial.println(" ");
      Serial.print("Length:");
      Serial.println(len,HEX);
#endif

    message = tmp;
    available = true;
  }
  cres.resetDecoder();
}
#endif

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
      // update uptime. Could be every where, but still put here
      uptime(millis(), false);

      if (rc == false) {
        // rc = next decoder;
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
    Serial.println(F("? Use one of V t R q"));
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

String hex2bin(String hexaDecimal)
{
  byte i=0;
  String bitstream;

  while(hexaDecimal[i]){
    switch(hexaDecimal[i]){
      case '0': bitstream += "0000"; break;
      case '1': bitstream += "0001"; break;
      case '2': bitstream += "0010"; break;
      case '3': bitstream += "0011"; break;
      case '4': bitstream += "0100"; break;
      case '5': bitstream += "0101"; break;
      case '6': bitstream += "0110"; break;
      case '7': bitstream += "0111"; break;
      case '8': bitstream += "1000"; break;
      case '9': bitstream += "1001"; break;
      case 'A': bitstream += "1010"; break;
      case 'B': bitstream += "1011"; break;
      case 'C': bitstream += "1100"; break;
      case 'D': bitstream += "1101"; break;
      case 'E': bitstream += "1110"; break;
      case 'F': bitstream += "1111"; break;
      case 'a': bitstream += "1010"; break;
      case 'b': bitstream += "1011"; break;
      case 'c': bitstream += "1100"; break;
      case 'd': bitstream += "1101"; break;
      case 'e': bitstream += "1110"; break;
      case 'f': bitstream += "1111"; break;
      default:  ;
    }
    i++;
  }
  return bitstream;
}
