/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mdorenka + jowiemann
// see http://forum.fhem.de/index.php/topic,17196.0.html
//
// History of changes:
// 2013-11-07 - started working on PCA 301
// 2013-12-06 - second version
// 2013-12-15 - KW9010ISR
// 2013-12-15 - fixed a bug where readings did not get submitted due to wrong error
// 2013-12-18 - major upgrade to new receiver system (might be extendable to other protocols
// 2013-12-18 - fixed typo that prevented compilation of code
// 2014-02-27 - remove magic numbers for IO Pins, #define constants instead / Make the code Leonardo compatible / This has not been tested on a atmega328 (all my devices are atmega32u4 based)
// 2014-04-09 - add support for pearl NC7159 Code from http://forum.fhem.de/index.php/topic,17196.msg147168.html#msg147168 Sensor from: http://www.pearl.de/a-NC7159-3041.shtml
// 2014-06-04 - add support for EUROCHRON modified Code from http://forum.arduino.cc/index.php/topic,136836.0.html / Prevent receive the same message continius
// 2014-06-11 - add support for LogiLink WS0002
// 2014-06-13 - EUROCHRON bugfix for neg temp
// 2014-06-16 - Added TX70DTH (Aldi)
// 2014-06-16 - Added DCF77 ( http://www.arduinoclub.de/2013/11/15/dcf77-dcf1-arduino-pollin/)
// 2014-06-18 - Two loops for different duration timings
// 2014-06-21 - Added basic Support to dectect the follwoing codecs: Oregon Scientific v2, Oregon Scientific v3,Cresta,Kaku,XRF,Home Easy
//            - Implemented Decoding for OSV2 Protocol
//            - Added some compiler switches for DCF-77, but they are currently not working
//            - Optimized duration calculation and saved variable 'time'.
// 2014-06-22 - Added Compiler Switch for __AVR_ATmega32U4__ DCF Pin#2

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduino"
#define PROGVERS               "2.0d"

#define COMP_DCF77    // Compile sketch witdh DCF-77 Support (currently disableling this is not working, has still to be done)
#define COMP_OSV2     // Compile sketch with OSV2 Support
#define COMP_Cresta     // Compile sketch with Cresta Support (currently not implemented, just for future use)

// Future enhancement
//#define COMP_OSV3     // Compile sketch with OSV3 Support (currently not implemented, just for future use)

//#define COMP_Kaku     // Compile sketch with Kaku  Support (currently not implemented, just for future use)
//#define COMP_HEZ     // Compile sketch with Homeeasy Support (currently not implemented, just for future use)
//#define COMP_XRF     // Compile sketch with XTF Support (currently not implemented, just for future use)



#ifdef COMP_DCF77
/*
 * DCF77_SerialTimeOutput
 * Ralf Bohnen, 2013
 * This example code is in the public domain.
 */
#include "DCF77.h"
#include "Time.h"

char time_s[9];
char date_s[11];

#if defined(__AVR_ATmega32U4__)          //on the leonardo and other ATmega32U4 devices interrupt 1 is on dpin 2
  #define DCF_PIN 2            // Connection pin to DCF 77 device
#else
  #define DCF_PIN 3            // Connection pin to DCF 77 device
#endif

#define DCF_INTERRUPT 1      // Interrupt number associated with pin

time_t time;
DCF77 DCF = DCF77(DCF_PIN, DCF_INTERRUPT);

bool    IS_DCF77               = true;      // set to true if DCF77 is connected

/*
 *
 *
 *
 */
#endif





/*
 * Modified code to fit info fhemduino - Sidey
 * Oregon V2 decoder modfied - Olivier Lebrun
 * Oregon V2 decoder added - Dominique Pierre
 * New code to decode OOK signals from weather sensors, etc.
 * 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
 *
 */


/* Currently not working, this is for future Mode
#include "decoders.h";
#ifdef COMP_OSV2     // Compile sketch with OSV3 Support (currently not implemented, just for future use)
OregonDecoderV2 orscV2;
#endif

#ifdef COMP_Cresta     // Compile sketch with Cresta Support (currently not implemented, just for future use)
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

*/







class DecodeOOK {
  protected:
    byte total_bits, bits, flip, state, pos, data[25];

    virtual char decode (word width) = 0;

  public:

    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

    DecodeOOK () {
      resetDecoder();
    }

    bool nextPulse (word width) {
      if (state != DONE)

        switch (decode(width)) {
          case -1: resetDecoder(); break;
          case 1:  done(); break;
        }
      return isDone();
    }

    bool isDone () const {
      return state == DONE;
    }

    const byte* getData (byte& count) const {
      count = pos;
      return data;
    }

    void resetDecoder () {
      total_bits = bits = pos = flip = 0;
      state = UNKNOWN;
    }

    // add one bit to the packet data buffer

    virtual void gotBit (char value) {
      total_bits++;
      byte *ptr = data + pos;
      *ptr = (*ptr >> 1) | (value << 7);

      if (++bits >= 8) {
        bits = 0;
        if (++pos >= sizeof data) {
          resetDecoder();
          return;
        }
      }
      state = OK;
    }

    // store a bit using Manchester encoding
    void manchester (char value) {
      flip ^= value; // manchester code, long pulse flips the bit
      gotBit(flip);
    }

    // move bits to the front so that all the bits are aligned to the end
    void alignTail (byte max = 0) {
      // align bits
      if (bits != 0) {
        data[pos] >>= 8 - bits;
        for (byte i = 0; i < pos; ++i)
          data[i] = (data[i] >> bits) | (data[i + 1] << (8 - bits));
        bits = 0;
      }
      // optionally shift bytes down if there are too many of 'em
      if (max > 0 && pos > max) {
        byte n = pos - max;
        pos = max;
        for (byte i = 0; i < pos; ++i)
          data[i] = data[i + n];
      }
    }

    void reverseBits () {
      for (byte i = 0; i < pos; ++i) {
        byte b = data[i];
        for (byte j = 0; j < 8; ++j) {
          data[i] = (data[i] << 1) | (b & 1);
          b >>= 1;
        }
      }
    }

    void reverseNibbles () {
      for (byte i = 0; i < pos; ++i)
        data[i] = (data[i] << 4) | (data[i] >> 4);
    }

    void done () {
      while (bits)
        gotBit(0); // padding
      state = DONE;
    }
};

#ifdef COMP_OSV2
class OregonDecoderV2 : public DecodeOOK {
  public:

    OregonDecoderV2() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      if (!(total_bits & 0x01))
      {
        data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      }
      total_bits++;
      pos = total_bits >> 4;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        //Serial.print("Dauer="); Serial.println(width);
        //Serial.println(width);
        byte w = width >= 700;

        switch (state) {
          case UNKNOWN:
            if (w != 0) {
              // Long pulse
              ++flip;
            } else if (w == 0 && 24 <= flip) {
              // Short pulse, start bit
              flip = 0;
              state = T0;
            } else {
              // Reset decoder
              return -1;
            }
            break;
          case OK:
            if (w == 0) {
              // Short pulse
              state = T0;
            } else {
              // Long pulse
              manchester(1);
            }
            break;
          case T0:
            if (w == 0) {
              // Second short pulse
              manchester(0);
            } else {
              // Reset decoder
              return -1;
            }
            break;
        }
      } else if (width >= 2500  && pos >= 8) {
        return 1;
      } else {
        return -1;
      }
      return 0;
    }
};
OregonDecoderV2 orscV2;
#endif

#ifdef COMP_OSV3
class OregonDecoderV3 : public DecodeOOK {
  public:
    OregonDecoderV3() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      total_bits++;
      pos = total_bits >> 3;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
            if (w == 0)
              ++flip;
            else if (32 <= flip) {
              flip = 1;
              manchester(1);
            } else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              manchester(1);
            break;
          case T0:
            if (w == 0)
              manchester(0);
            else
              return -1;
            break;
        }
      } else {
        return -1;
      }
      return  total_bits == 80 ? 1 : 0;
    }
};
OregonDecoderV3 orscV3;
#endif

#ifdef COMP_Cresta
class CrestaDecoder : public DecodeOOK {
  public:
    CrestaDecoder () {}

    const byte* getData (byte& count) const {

      count = pos;
      return data;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1300) {
        byte w = width >= 750;
        switch (state) {
          case UNKNOWN:
            if (w == 1)
              ++flip;
            else if (2 <= flip && flip <= 10)
              state = T0;
            else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              gotBit(1);
            break;
          case T0:
            if (w == 0)
              gotBit(0);
            else
              return -1;
            break;
        }
      } else if (width >= 2500 && pos >= 7)
        return 1;
      else
        return -1;
      return 0;
    }

    virtual void gotBit (char value) {

      if (++bits <= 8) {

        total_bits++;
        byte *ptr = data + pos;
        *ptr = (*ptr >> 1) | (value << 7);
      }
      else {

        bits = 0;
        if (++pos >= sizeof data) {
          resetDecoder();
          return;
        }
      }
      state = OK;
    }


};
CrestaDecoder cres;
#endif

#ifdef COMP_KAKU
class KakuDecoder : public DecodeOOK {
  public:
    KakuDecoder () {}

    virtual char decode (word width) {
      if (180 <= width && width < 450 || 950 <= width && width < 1250) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            if (w)
              state = T1;
            else
              return -1;
            break;
          case T1:
            state += w + 1;
            break;
          case T2:
            if (w)
              gotBit(0);
            else
              return -1;
            break;
          case T3:
            if (w == 0)
              gotBit(1);
            else
              return -1;
            break;
        }
      } else if (width >= 2500 && 8 * pos + bits == 12) {
        for (byte i = 0; i < 4; ++i)
          gotBit(0);
        alignTail(2);
        return 1;
      } else
        return -1;
      return 0;
    }
};
KakuDecoder kaku;
#endif

#ifdef COMP_XRF
class XrfDecoder : public DecodeOOK {
  public:
    XrfDecoder () {}

    // see also http://davehouston.net/rf.htm
    virtual char decode (word width) {
      if (width > 2000 && pos >= 4)
        return 1;
      if (width > 5000)
        return -1;
      if (width > 4000 && state == UNKNOWN)
        state = OK;
      else if (350 <= width && width < 1800) {
        byte w = width >= 720;
        switch (state) {
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            gotBit(w);
            break;
        }
      } else
        return -1;
      return 0;
    }
};
XrfDecoder xrf;
#endif

#ifdef COMP_HEZ
class HezDecoder : public DecodeOOK {
  public:
    HezDecoder () {}

    // see also http://homeeasyhacking.wikia.com/wiki/Home_Easy_Hacking_Wiki
    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 600;
        gotBit(w);
      } else if (width >= 5000 && pos >= 5 /*&& 8 * pos + bits == 50*/) {
        for (byte i = 0; i < 6; ++i)
          gotBit(0);
        alignTail(7); // keep last 56 bits
        return 1;
      } else
        return -1;
      return 0;
    }
};
HezDecoder hez;
#endif


/*
*
*
*
*/


#define MAX_CHANGES            75
#define BAUDRATE               9600
#define RECEIVETOLERANCE       60
#define PIN_LED                13
#define PIN_SEND               11
#if defined(__AVR_ATmega32U4__)          //on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
#define PIN_RECEIVE            3
#else
#define PIN_RECEIVE            2
#endif
#define STARTBIT_TIME          5000
#define STARTBIT_OFFSET        200
#define STARTBIT_TIME2         2500
#define STARTBIT_OFFSET2       100

bool    DEBUG                  = true;      // set to true to see debug messages
bool    EQ_BIT_STREAM_ALLOW    = true;      // set to true to allow equal bit stream in sequence
bool    IS_WS0002              = true;      // set to true for WS0002, false for NC7159

unsigned int timings[MAX_CHANGES];
unsigned int timings2[MAX_CHANGES];
String cmdstring;
volatile bool available = false;
String message = "";
long NC7159_bitsequence, NC7159_bitseqsave, EuroChron_bitsequence, EuroChron_bitseqsave, KW9010_bitsequence, KW9010_bitseqsave, WS0002_bitsequence, WS0002_bitseqsave;
long TX70DTH_bitsequence, TX70DTH_bitseqsave;

void setup() {
  // put your setup code here, to run once:
  delay(4000);
  Serial.begin(BAUDRATE);
  enableReceive();
  pinMode(PIN_RECEIVE, INPUT);
  pinMode(PIN_SEND, OUTPUT);

#ifdef COMP_DCF77
  if (IS_DCF77) {
    DCF.Start();
    if (DEBUG) {
      Serial.println("Warte auf Zeitsignal ... ");
      Serial.println("Dies kann 2 oder mehr Minuten dauern.");
    }
  }
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
  if (messageAvailable()) {
    Serial.println(message);
    resetAvailable();
  }

#ifdef COMP_DCF77
  if (IS_DCF77) {
    time_t DCFtime = DCF.getTime(); // Nachschauen ob eine neue DCF77 Zeit vorhanden ist
    if (DCFtime != 0)
    {
      setTime(DCFtime); //Neue Systemzeit setzen
      // Serial.print("Neue Zeit erhalten : "); //Ausgabe an seriell
      Serial.print("D");
      Serial.print(sprintTime());
      Serial.print("-");
      Serial.println(sprintDate());
    }
  }
#endif

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
  attachInterrupt(0, handleInterrupt, CHANGE);
}

void disableReceive() {
  detachInterrupt(0);
}

void handleInterrupt() {
  static unsigned int duration;
  static unsigned long lastTime;

  //long time = micros();
  duration = micros() - lastTime;

  Startbit_5000(duration);
  Startbit_2500(duration);

#ifdef COMP_OSV2
  if (orscV2.nextPulse(duration) )
  {
    byte len;
    const byte* data = orscV2.getData(len);

    char tmp[36]="";
    int tmp_len = 0;
    strcat(tmp, "OSV2:");
    tmp_len = 5;
    if (DEBUG)
    {
      Serial.print("HEXStream");
    }
    for (byte i = 0; i < len; ++i) {
      if (DEBUG)
      {
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
        Serial.print(",");
      }
      tmp_len += snprintf(tmp + tmp_len, 36, "%X", data[i]);
    }
    if (DEBUG)
    {
      Serial.println(" ");
    }

    message = tmp;
    available = true;
    orscV2.resetDecoder();
  }
#endif

#ifdef COMP_Cresta
 
  if (cres.nextPulse(duration))
  {
    byte len;
    const byte* data = orscV2.getData(len) + 5;
    char tmp[36]="";
    int tmp_len = 0;
    strcat(tmp, "CRESTA:");
    tmp_len = 7;
    if (DEBUG)
    {
      Serial.print("HEXStream");
    }
    for (byte i = 0; i < len; ++i) {
      if (DEBUG)
      {
        Serial.print(data[i] >> 4, HEX);
        Serial.print(data[i] & 0x0F, HEX);
        Serial.print(",");
      }
      tmp_len += snprintf(tmp + tmp_len, 36, "%X", data[i]);
    }
    if (DEBUG)
    {
      Serial.println(" ");
    }

    message = tmp;
    available = true;
  }
  cres.resetDecoder();
#endif

  lastTime += duration;
}

void Startbit_5000(unsigned int duration) {
  static unsigned int changeCount;
  static unsigned int repeatCount;

  if (duration > STARTBIT_TIME && duration > timings[0] - STARTBIT_OFFSET && duration < timings[0] + STARTBIT_OFFSET) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      if (receiveProtocolKW9010(changeCount) == false) {
        if (receiveProtocolPT2262(changeCount) == false) {
          if (receiveProtocolNC7159(changeCount) == false) {
            if (receiveProtocolWS0002(changeCount) == false) {
              if (receiveProtocolEuroChron(changeCount) == false) {
                // failed
              }
            }
          }
        }
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

void Startbit_2500(unsigned int duration) {
  static unsigned int changeCount;
  static unsigned int repeatCount;

  if (duration > STARTBIT_TIME2 && duration > timings2[0] - STARTBIT_OFFSET2 && duration < timings2[0] + STARTBIT_OFFSET2) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
      if (receiveProtocolTX70DTH(changeCount) == false) {
        // failed
      }
      repeatCount = 0;
    }
    changeCount = 0;
  }
  else if (duration > STARTBIT_TIME2) {
    changeCount = 0;
  }

  if (changeCount >= MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  timings2[changeCount++] = duration;
}

/*
 * Serial Command Handling
 */
void serialEvent()
{
  while (Serial.available())
  {


    char inChar = (char)Serial.read();
    switch (inChar)
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
    digitalWrite(PIN_LED, HIGH);
    char msg[13];
    cmd.substring(2).toCharArray(msg, 13);
    sendPT2262(msg);
    digitalWrite(PIN_LED, LOW);
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
  int i = 0;

  if ((timings[0] < KW9010_SYNC - KW9010_GLITCH) || (timings[0] > KW9010_SYNC + KW9010_GLITCH)) {
    return false;
  }
  //Serial.println(changeCount);
  for (int i = 2; i < changeCount; i = i + 2) {
    if ((timings[i] > KW9010_ZERO - KW9010_GLITCH) && (timings[i] < KW9010_ZERO + KW9010_GLITCH)) {
      // its a zero
      bitmessage[bitcount] = false;
      KW9010_bitsequence <<= 1;
      bitcount++;
    }
    else if ((timings[i] > KW9010_ONE - KW9010_GLITCH) && (timings[i] < KW9010_ONE + KW9010_GLITCH)) {
      // its a one
      bitmessage[bitcount] = true;
      KW9010_bitsequence <<= 1;
      KW9010_bitsequence |= 1;
      bitcount++;
    }
    else {
      return false;
    }
  }

  if (DEBUG) {
    Serial.print("Bit-Stream: ");
    for (i = 0; i < KW9010_MESSAGELENGTH; i++) {
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  }

  // Test if bitsequence was previously received
  if ((KW9010_bitsequence == KW9010_bitseqsave) && EQ_BIT_STREAM_ALLOW) {
    return false;
  }
  else {
    KW9010_bitseqsave = KW9010_bitsequence;
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

  for ( i = 0 ; i <= 7 ; i++) {
    calculatedChecksum += (byte)(bitmessage[i * 4 + 3] << 3 | bitmessage[i * 4 + 2] << 2 | bitmessage[i * 4 + 1] << 1 | bitmessage[i * 4]);
  }
  calculatedChecksum &= 0xF;

  if (calculatedChecksum == checksum) {
    if (temperature > -500 && temperature < 700) {
      if (humidity > 0 && humidity < 100) {
        char tmp[11];
        sprintf(tmp, "K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
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

  for (int i = 1; i < changeCount; i = i + 2) {
    if (timings[i] > delay - delayTolerance && timings[i] < delay + delayTolerance && timings[i + 1] > delay * 3 - delayTolerance && timings[i + 1] < delay * 3 + delayTolerance) {
      code = code << 1;
    }
    else if (timings[i] > delay * 3 - delayTolerance && timings[i] < delay * 3 + delayTolerance && timings[i + 1] > delay - delayTolerance && timings[i + 1] < delay + delayTolerance)  {
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

/*
 * NC7159
 */

bool receiveProtocolNC7159(unsigned int changeCount) {
#define NC7159_SYNC   9000
#define NC7159_ONE    3750
#define NC7159_ZERO   1800
#define NC7159_GLITCH  250
#define NC7159_MESSAGELENGTH 36

  bool bitmessage[36];
  int i = 0;

  if (IS_WS0002) {
    return false;
  }

  if (changeCount < NC7159_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings[0] < NC7159_SYNC - NC7159_GLITCH) || (timings[0] > NC7159_SYNC + NC7159_GLITCH)) {
    return false;
  }

  for (i = 0; i < (NC7159_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings[i + 2] > NC7159_ZERO - NC7159_GLITCH) && (timings[i + 2] < NC7159_ZERO + NC7159_GLITCH))    {
      bitmessage[i >> 1] = false;
      NC7159_bitsequence <<= 1;
    }
    else if ((timings[i + 2] > NC7159_ONE - NC7159_GLITCH) && (timings[i + 2] < NC7159_ONE + NC7159_GLITCH)) {
      bitmessage[i >> 1] = true;
      NC7159_bitsequence <<= 1;
      NC7159_bitsequence |= 1;
    }
    else {
      return false;
    }
  }

  if (DEBUG) {
    Serial.print("Bit-Stream: ");
    for (i = 0; i < NC7159_MESSAGELENGTH; i++) {
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  }

  // Test if bitsequence was previously received
  if ((NC7159_bitsequence == NC7159_bitseqsave) && EQ_BIT_STREAM_ALLOW) {
    return false;
  }
  else {
    NC7159_bitseqsave = NC7159_bitsequence;
  }

  // Sensor ID & Channel
  byte unsigned id = bitmessage[3] | bitmessage[2] << 1 | bitmessage[1] << 2 | bitmessage[0] << 3 ;
  if (id != 5) {
    return false;
  }

  id = 0; // unterdruecke Bit 4+5, jetzt erst einmal nur 6 Bit
  for (i = 6; i < 12; i++)  if (bitmessage[i]) id +=  1 << (13 - i);

  // Bit 12 : immer 1 oder doch Battery State ?
  bool battery = !bitmessage[12];

  // Bit 14 + 15 = Kanal  0 - 2 , id nun bis auf 8 Bit fuellen
  id = id | bitmessage[14] << 1 | bitmessage[15] ;

  // Trigger
  bool forcedSend = bitmessage[13];

  int temperature = 0;
  for (i = 17; i < 28; i++) if (bitmessage[i]) temperature +=  1 << (27 - i);
  if (bitmessage[16]) temperature -= 2048; // negative Temp

  // die restlichen 8 Bits sind z.Z unbekannt vllt. eine Pruefsumme ?
  byte rest = 0;
  for (i = 28; i < 36; i++) if (bitmessage[i]) rest +=  1 << (35 - i);

  char tmp[11];
  sprintf(tmp, "K%02x%01d%01d%01d%+04d%02d", id, battery, 0, forcedSend, temperature, rest);
  message = tmp;
  available = true;
  return true;
}

/*
 * WS0002 / LogiLink WS0002
 */

bool receiveProtocolWS0002(unsigned int changeCount) {
#define WS0002_SYNC   9000
#define WS0002_ONE    3750
#define WS0002_ZERO   1800
#define WS0002_GLITCH  250
#define WS0002_MESSAGELENGTH 36

  bool bitmessage[36];
  int i = 0;

  if (!IS_WS0002) {
    return false;
  }
  if (changeCount < WS0002_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings[0] < WS0002_SYNC - WS0002_GLITCH) || (timings[0] > WS0002_SYNC + WS0002_GLITCH)) {
    return false;
  }

  for (i = 0; i < (WS0002_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings[i + 2] > WS0002_ZERO - WS0002_GLITCH) && (timings[i + 2] < WS0002_ZERO + WS0002_GLITCH))    {
      bitmessage[i >> 1] = false;
      WS0002_bitsequence <<= 1;
    }
    else if ((timings[i + 2] > WS0002_ONE - WS0002_GLITCH) && (timings[i + 2] < WS0002_ONE + WS0002_GLITCH)) {
      bitmessage[i >> 1] = true;
      WS0002_bitsequence <<= 1;
      WS0002_bitsequence |= 1;
    }
    else {
      return false;
    }
  }

  //                 /--------------------------------- Sensdortype
  //                /    / ---------------------------- ID, changes after every battery change
  //               /    /        /--------------------- Battery state 0 == Ok
  //              /    /        /  / ------------------ forced send
  //             /    /        /  /  / ---------------- Channel (1..3)
  //            /    /        /  /  /  / -------------- neg Temp: if 1 then temp = temp - 2048
  //           /    /        /  /  /  /   / ----------- Temp
  //          /    /        /  /  /  /   /          /-- unknown
  //         /    /        /  /  /  /   /          /  / Humidity
  //         0101 00101001 0  0  00 0  01000110000 1  1011101
  // Bit     0    4        12 13 14 16 17          28 29    36

  if (DEBUG) {
    Serial.print("WS0002: ");
    for (i = 0; i < NC7159_MESSAGELENGTH; i++) {  // Todo, change to correct definition
      if (i == 4) Serial.print(" ");
      if (i == 12) Serial.print(" ");
      if (i == 13) Serial.print(" ");
      if (i == 14) Serial.print(" ");
      if (i == 16) Serial.print(" ");
      if (i == 17) Serial.print(" ");
      if (i == 28) Serial.print(" ");
      if (i == 29) Serial.print(" ");
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  }

  // Test if bitsequence was previously received
  if ((WS0002_bitsequence != WS0002_bitseqsave) || EQ_BIT_STREAM_ALLOW) {
    WS0002_bitseqsave = WS0002_bitsequence;
  }
  else {
    return false;
  }

  // Sensor ID & Channel
  byte unsigned id = bitmessage[3] | bitmessage[2] << 1 | bitmessage[1] << 2 | bitmessage[0] << 3 ;
  if (id != 5) {
    return false;
  }

  id = 0; // 4-11 ZufallsID nach einlegen Batterie
  for (i = 4; i < 12; i++)  if (bitmessage[i]) id +=  1 << (13 - i);

  // Bit 12 : Battery State
  bool battery = !bitmessage[12];

  // Bit 13 : Trigger
  bool forcedSend = bitmessage[13];

  // Bit 14 + 15 = Kanal  0 - 2 , id nun bis auf 8 Bit fuellen
  byte unsigned channel = bitmessage[15] | bitmessage[14]  << 1;

  // Bit 16 : Vorzeichen Temperatur

  // Temperatur
  int temperature = 0;
  for (i = 17; i < 28; i++) if (bitmessage[i]) temperature +=  1 << (27 - i);
  if (bitmessage[16]) temperature -= 2048; // negative Temp

  // Don't know ?
  byte trend = 0;
  for (i = 28; i < 29; i++)  if (bitmessage[i]) trend +=  1 << (29 - i);

  // die restlichen 6 Bits fuer Luftfeuchte
  int humidity = 0;
  for (i = 29; i < 36; i++) if (bitmessage[i]) humidity +=  1 << (35 - i);

  char tmp[13];
  sprintf(tmp, "L%01d%02x%01d%01d%01d%+04d%02d", channel, id, battery, trend, forcedSend, temperature, humidity);
  message = tmp;
  available = true;
  return true;
}

/*
 * EUROCHRON
 */

bool receiveProtocolEuroChron(unsigned int changeCount) {
#define EuroChron_SYNC   8050
#define EuroChron_ONE    2020
#define EuroChron_ZERO   1010
#define EuroChron_GLITCH  100
#define EuroChron_MESSAGELENGTH 36

  bool bitmessage[36];
  int i = 0;

  if (changeCount < EuroChron_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings[0] < EuroChron_SYNC - EuroChron_GLITCH) || (timings[0] > EuroChron_SYNC + EuroChron_GLITCH)) {
    return false;
  }

  for (i = 0; i < (EuroChron_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings[i + 2] > EuroChron_ZERO - EuroChron_GLITCH) && (timings[i + 2] < EuroChron_ZERO + EuroChron_GLITCH))    {
      bitmessage[i >> 1] = false;
      EuroChron_bitsequence <<= 1;
    }
    else if ((timings[i + 2] > EuroChron_ONE - EuroChron_GLITCH) && (timings[i + 2] < EuroChron_ONE + EuroChron_GLITCH)) {
      bitmessage[i >> 1] = true;
      EuroChron_bitsequence <<= 1;
      EuroChron_bitsequence |= 1;
    }
    else {
      return false;
    }
  }

  //                /--------------------------- Channel, changes after every battery change
  //               /        / ------------------ Battery state 0 == Ok
  //              /        / /------------------ unknown
  //             /        / /  / --------------- forced send
  //            /        / /  /  / ------------- unknown
  //           /        / /  /  /     / -------- Humidity
  //          /        / /  /  /     /       / - neg Temp: if 1 then temp = temp - 2048
  //         /        / /  /  /     /       /  / Temp
  //         01100010 1 00 1  00000 0100011 0  00011011101
  // Bit     0        8 9  11 12    17      24 25        36

  if (DEBUG) {
    Serial.print("Bit-Stream: ");
    for (i = 0; i < EuroChron_MESSAGELENGTH; i++) {
      if (i == 8) Serial.print(" ");
      if (i == 9) Serial.print(" ");
      if (i == 11) Serial.print(" ");
      if (i == 12) Serial.print(" ");
      if (i == 17) Serial.print(" ");
      if (i == 24) Serial.print(" ");
      if (i == 25) Serial.print(" ");
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  }

  // Test if bitsequence was previously received
  if ((EuroChron_bitsequence != EuroChron_bitseqsave) || EQ_BIT_STREAM_ALLOW) {
    EuroChron_bitseqsave = EuroChron_bitsequence;
  }
  else {
    return false;
  }

  // Sensor ID & Channel, will be changed after every battery change
  byte unsigned id = 0;
  for (i = 0; i < 8; i++)  if (bitmessage[i]) id +=  1 << (7 - i);

  // Battery State
  bool battery = bitmessage[8];

  // first unknown
  byte firstunknown = 0;
  for (i = 9; i < 11; i++)  if (bitmessage[i]) firstunknown +=  1 << (10 - i);

  // Trigger
  bool forcedSend = bitmessage[11];

  // second unknown
  byte secunknown = 0;
  for (i = 12; i < 17; i++)  if (bitmessage[i]) secunknown +=  1 << (16 - i);

  // Luftfeuchte
  int humidity = 0;
  for (i = 17; i < 24; i++) if (bitmessage[i]) humidity +=  1 << (23 - i);

  // Temperatur
  int temperature = 0;
  for (i = 25; i < 36; i++) if (bitmessage[i]) temperature +=  1 << (35 - i);
  if (bitmessage[24]) temperature -= 2048; // negative Temp

  char tmp[14];
  sprintf(tmp, "T%02x%01d%01d%01d%02d%+04d%02d", id, battery, firstunknown, forcedSend, secunknown, temperature, humidity);
  message = tmp;
  available = true;
  return true;
}

bool receiveProtocolTX70DTH(unsigned int changeCount) {

#define TX70DTH_SYNC   4000
#define TX70DTH_ONE    2030
#define TX70DTH_ZERO   1020
#define TX70DTH_GLITCH  250
#define TX70DTH_MESSAGELENGTH 36

  bool bitmessage[36];
  byte i;
  if (changeCount < TX70DTH_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings2[0] < TX70DTH_SYNC - TX70DTH_GLITCH) || (timings2[0] > TX70DTH_SYNC + TX70DTH_GLITCH)) {
    return false;
  }
  for (i = 0; i < (TX70DTH_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings2[i + 2] > TX70DTH_ZERO - TX70DTH_GLITCH) && (timings2[i + 2] < TX70DTH_ZERO + TX70DTH_GLITCH))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings2[i + 2] > TX70DTH_ONE - TX70DTH_GLITCH) && (timings2[i + 2] < TX70DTH_ONE + TX70DTH_GLITCH)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }

  //                /--------------------------------- Channel, changes after every battery change
  //               /        / ------------------------ Battery state 1 == Ok
  //              /        / /------------------------ Kanal 000 001 010 hier (Kanal 3)
  //             /        / /   / -------------------- neg Temp: normal = 000 if 111 then temp = temp - 512
  //            /        / /   /   / ----------------- Temp
  //           /        / /   /   /         / -------- filler also 1111
  //          /        / /   /   /         /     /---- Humidity
  //         /        / /   /   /         /     /
  //         11111101 1 010 000 011111001 1111 00101111
  // Bit     0        8 9   12  15        24   28     35
  if (DEBUG) {
    Serial.print("Bit-Stream: ");
    for (i = 0; i < TX70DTH_MESSAGELENGTH; i++) {
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  }

  // Sensor ID & Channel
  byte unsigned id = bitmessage[3] | bitmessage[2] << 1 | bitmessage[1] << 2 | bitmessage[0] << 3 ;
  id = 0; // unterdruecke Bit 4+5, jetzt erst einmal nur 6 Bit
  for (i = 6; i < 12; i++)  if (bitmessage) id +=  1 << (13 - i);

  // Bit 9 : immer 1 oder doch Battery State ?
  bool battery = bitmessage[8];

  // Bit 11 + 12 = Kanal  0 - 2 , id nun bis auf 8 Bit fuellen
  id = id | bitmessage[10] << 1 | bitmessage[11] ;

  // Trigger
  bool forcedSend = 0; // wird nicht unterstützt
  byte trend = 0; //trend wird nicht unterstützt

  int temperature = 0;
  for (i = 16; i < 24; i++) if (bitmessage[i]) temperature +=  1 << (23 - i);
  if (bitmessage[14]) temperature -= 0x200; // negative Temp
  byte feuchte = 0;
  for (i = 29; i < 36; i++) if (bitmessage[i]) feuchte +=  1 << (35 - i);

  // die restlichen 4 Bits sind z.Z unbekannt
  byte rest = 0;
  for (i = 24; i < 27; i++) if (bitmessage[i]) rest +=  1 << (26 - i);

  char tmp[11];
  sprintf(tmp, "K%02x%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, feuchte);
  message = tmp;
  available = true;
  return true;
}

void sendPT2262(char* triStateMessage) {
  for (int i = 0; i < 3; i++) {
    unsigned int pos = 0;
    while (triStateMessage[pos] != '\0') {
      switch (triStateMessage[pos]) {
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
  PT2262_transmit(1, 3);
  PT2262_transmit(1, 3);
}

void PT2262_sendT1() {
  PT2262_transmit(3, 1);
  PT2262_transmit(3, 1);
}

void PT2262_sendTF() {
  PT2262_transmit(1, 3);
  PT2262_transmit(3, 1);
}

void PT2262_sendSync() {
  PT2262_transmit(1, 31);
}

void PT2262_transmit(int nHighPulses, int nLowPulses) {
  disableReceive();
  digitalWrite(PIN_SEND, HIGH);
  delayMicroseconds(350 * nHighPulses);
  digitalWrite(PIN_SEND, LOW);
  delayMicroseconds(350 * nLowPulses);
  enableReceive();
}

#ifdef COMP_DCF77
char* sprintTime() {
  //    char tmp[6];
  //    snprintf(tmp, "%02d%02d%02d", hour(), minute(), second());
  //    return tmp;
  snprintf(time_s, sizeof(time_s), "%02d%02d%02d" , hour(), minute(), second());
  time_s[strlen(time_s)] = '\0';
  return time_s;
}
#endif

#ifdef COMP_DCF77
char* sprintDate() {
  //    char tmp[8];
  //    snprintf(tmp, "%02d%02d%04d", day(), month(), year());
  //    return tmp;
  snprintf(date_s, sizeof(date_s), "%02d%02d%04d" , day(), month(), year());
  date_s[strlen(date_s)] = '\0';
  return date_s;
}

#endif
