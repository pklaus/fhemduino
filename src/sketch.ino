/*
 * EOF preprocessor bug prevent
*/
/// @dir FHEMduino (2013-11-07)
/// FHEMduino communicator
//
// authors: mdorenka + jowiemann + sidey, mick6300
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
// 2014-06-11 - add support for LogiLink NC_WS
// 2014-06-13 - EUROCHRON bugfix for neg temp 
// 2014-06-16 - Added TX70DTH (Aldi)
// 2014-06-16 - Added DCF77 ( http://www.arduinoclub.de/2013/11/15/dcf77-dcf1-arduino-pollin/)
// 2014-06-18 - Two loops for different duration timings
// 2014-06-21 - Added basic Support to dectect the follwoing codecs: Oregon Scientific v2, Oregon Scientific v3,Cresta,Kaku,XRF,Home Easy
//            - Implemented Decoding for OSV2 Protocol
//            - Added some compiler switches for DCF-77, but they are currently not working
//            - Optimized duration calculation and saved variable 'time'.
// 2014-06-22 - Added Compiler Switch for __AVR_ATmega32U4__ DCF Pin#2
// 2014-06-24 - Capsulatet all decoder with #defines
// 2014-06-24 - Added receive support for smoke detectors FA20RF / RM150RF (KD101 not verified)
// 2014-06-24 - Added send / activate support for smoke detectors FA20RF / RM150RF (KD101 not verified) -- not yet inetgrated in FHEM-Modul
// 2014-06-24 - Integrated mick6300 developments LIFETEC support
// 2014-06-24 - Integrated mick6300 developments TX70DTH (Aldi) support
// 2014-07-04 - Integrated Intertechno TX2/3/4 support, see CUL_TX and http://www.f6fbb.org/domo/sensors/tx3_th.php

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduino"
#define PROGVERS               "2.1d"    // Full versionstring is build in handle command function

#if defined(__AVR_ATmega32U4__)          // on the leonardo and other ATmega32U4 devices interrupt 0 is on dpin 3
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

//#define DEBUG           // Compile sketch witdh Debug informations
#ifdef DEBUG
#define BAUDRATE               115200
#else
#define BAUDRATE               9600
#endif


#define COMP_DCF77      // Compile sketch with DCF-77 Support (currently disableling this is not working, has still to be done)
#define COMP_PT2262     // Compile sketch with PT2262 (IT / ELRO switches)
#define COMP_FA20RF     // Compile sketch with smoke detector Flamingo FA20RF / ELRO RM150RF
#define COMP_KW9010     // Compile sketch with KW9010 support
#define COMP_NC_WS      // Compile sketch with PEARL NC7159, LogiLink WS0002 support
#define COMP_EUROCHRON  // Compile sketch with EUROCHRON / Tchibo support
#define COMP_LIFETEC    // Compile sketch with LIFETEC support
#define COMP_TX70DTH    // Compile sketch with TX70DTH (Aldi) support
#define COMP_IT_TX      // Compile sketch with Intertechno TX2/3/4 support

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

/* Currently not working, this is for future Mode
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
 * FA20RF
 */
static unsigned int FArepetition = 10;

/*
 * PT2262
 */
static unsigned int ITrepetition = 3;

/*
 * Weather sensors
 */
#define MAX_CHANGES            90
unsigned int timings5000[MAX_CHANGES];      //  Startbit_5000
unsigned int timings2500[MAX_CHANGES];      //  Startbit_2500

String cmdstring;
volatile bool available = false;
String message = "";

/*
 * DCF77_SerialTimeOutput
 * Ralf Bohnen, 2013
 * This example code is in the public domain.
 */
 
/*
 * BOF preprocessor bug prevent
 * insert me on top of your arduino-code
 */

#ifdef COMP_DCF77
#include "Time.h"        // Unterstuetzung für Datum/Zeit-Funktionen
#include "DCF77.h"
 
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

char* sprintTime() {
    snprintf(time_s,sizeof(time_s),"%02d%02d%02d" , hour(), minute(), second());
    time_s[strlen(time_s)] = '\0';
    return time_s;
}
 
char* sprintDate() {
    snprintf(date_s,sizeof(date_s),"%02d%02d%04d" , day(), month(), year());
    date_s[strlen(date_s)] = '\0';
    return date_s;
}
#endif

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

#ifdef COMP_DCF77
    DCF.Start();

#ifdef DEBUG
    Serial.println("Warte auf Zeitsignal ... ");
    Serial.println("Dies kann 2 oder mehr Minuten dauern.");
#endif

#endif // COMP_DCF77

}

void loop() {

  // put your main code here, to run repeatedly: 

  if (messageAvailable()) {
    Serial.println(message);
    resetAvailable();
  }

#ifdef COMP_DCF77
    time_t DCFtime = DCF.getTime(); // Nachschauen ob eine neue DCF77 Zeit vorhanden ist
    if (DCFtime!=0)
    {
      setTime(DCFtime); //Neue Systemzeit setzen
      // Serial.print("Neue Zeit erhalten : "); //Ausgabe an seriell
      Serial.print("D"); 
      Serial.print(sprintTime()); 
      Serial.print("-"); 
      Serial.println(sprintDate());   
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
  attachInterrupt(0,handleInterrupt,CHANGE);
}

void disableReceive() {
  detachInterrupt(0);
}

void handleInterrupt() {
  static unsigned int duration;
  static unsigned long lastTime;

  duration = micros() - lastTime;
  
#ifdef COMP_FA20RF
  FA20RF(duration);
#endif
#ifdef COMP_IT_TX
  IT_TX(duration);
#endif
  Startbit_5000(duration);
  Startbit_2500(duration);

#ifdef COMP_OSV2
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
// Dirty hack, just to test the 41_Oregon Module
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
#endif  //COMP_OSV2

#ifdef COMP_Cresta
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
#endif

  lastTime += duration;
}

#ifdef COMP_IT_TX
/*
 * TX Receiver
 */
#define TX_MAX_CHANGES 88
unsigned int timingsTX[TX_MAX_CHANGES+20];      //  TX

// http://www.f6fbb.org/domo/sensors/tx3_th.php
void IT_TX(unsigned int duration) {

  static unsigned int changeCount;
  static unsigned int fDuration;
  static unsigned int sDuration;

  sDuration = fDuration + duration;
  
  if ((sDuration > 1550 - 200 && sDuration < 1550 + 200) || (sDuration > 2150 - 200 && sDuration < 2150 + 200)) {
    if ((duration > 520 - 100 && duration < 520 + 100) || (duration > 1250 - 100 && duration < 1250 + 100) || (duration > 950 - 100 && duration < 950 + 100)) {
      if (changeCount == 0 ) {
//        Serial.print(changeCount);
//        Serial.print(" ");
//        Serial.println(fDuration);
        timingsTX[changeCount++] = fDuration;
      }
//      Serial.print(changeCount);
//      Serial.print(" ");
//      Serial.println(duration);
      timingsTX[changeCount++] = duration;
    }

    if ( changeCount == TX_MAX_CHANGES - 1) {
      receiveProtocolIT_TX(changeCount);
      changeCount = 0;
      fDuration = 0;
    } 
  } else {
    changeCount = 0;
  }

  fDuration = duration;
}

/*
 * Intertechno TX2_3 Decoder
 */
void receiveProtocolIT_TX(unsigned int changeCount) {
#define TX_ONE    520
#define TX_ZERO   1250
#define TX_GLITCH  100
#define TX_MESSAGELENGTH 44

  byte i;
  unsigned long code = 0;

  message = "TX";

  for (i = 0; i <= 14; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  
  // Startsequence 0000 1010 = 0xA
  if (code != 10) {
    Serial.println("Fehler");
    return;
  }
  code = 0;
  
  // Sensor type 0000 = Temp / 1110 = Humidity
  for (i = 16; i <= 22; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Sensor adress
  for (i = 24; i <= 38; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Temp or Humidity
  for (i = 40; i <= 62; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Repeated Bytes temp / Humidity
  for (i = 64; i <= 78; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      return;
    }
  }
  message += String(code,HEX);
  code = 0;

  // Checksum
  for (i = 80; i <= changeCount; i = i + 2)
  {
    if ((timingsTX[i] > TX_ZERO - TX_GLITCH) && (timingsTX[i] < TX_ZERO + TX_GLITCH))    {
      code <<= 1;
    }
    else if ((timingsTX[i] > TX_ONE - TX_GLITCH) && (timingsTX[i] < TX_ONE + TX_GLITCH)) {
      code <<= 1;
      code |= 1;
    }
    else {
      Serial.println("F");
      return;
    }
  }
  message += String(code,HEX);
  
  message.toUpperCase();

 // Serial.print("TX2_3: ");
 // Serial.println(i);

  available = true;
  return;
}
#endif

#ifdef COMP_FA20RF
/*
 * FA20RF Receiver
 */
#define FA20_MAX_CHANGES 60
unsigned int timingsFA20[FA20_MAX_CHANGES];      //  FA20RF

void FA20RF(unsigned int duration) {
#define L_STARTBIT_TIME         8020
#define H_STARTBIT_TIME         8120
#define L_STOPBIT_TIME          10000
#define H_STOPBIT_TIME          14500

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
#ifdef DEBUG
    Serial.print("changeCount: ");
    Serial.println(changeCount);
#endif
    return;
  }
  
  if ((timingsFA20[0] < FA20RF_SYNC - FA20RF_GLITCH) || (timingsFA20[0] > FA20RF_SYNC + FA20RF_GLITCH)) {
#ifdef DEBUG
    Serial.print("timingsFA20[0]: ");
    Serial.println(timingsFA20[0]);
#endif
    return;
  }

  if ((timingsFA20[1] < FA20RF_SYNC2 - FA20RF_GLITCH) || (timingsFA20[1] > FA20RF_SYNC2 + FA20RF_GLITCH)) {
#ifdef DEBUG
    Serial.print("timingsFA20[1]: ");
    Serial.println(timingsFA20[1]);
#endif
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

void sendFA20RF(char* triStateMessage) {
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
    while (triStateMessage[pos] != '\0') {
      switch(triStateMessage[pos]) {
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
  Serial.print("Ende Senden: ");
  Serial.println(pos);
}
#endif

/*
 * decoders with an startbit > 5000
 */
void Startbit_5000(unsigned int duration) {
#define STARTBIT_TIME   5000
#define STARTBIT_OFFSET 200

  static unsigned int changeCount;
  static unsigned int repeatCount;
  bool rc = false;

  if (duration > STARTBIT_TIME && duration > timings5000[0] - STARTBIT_OFFSET && duration < timings5000[0] + STARTBIT_OFFSET) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
#ifdef DEBUG
      Serial.print("changeCount: ");
      Serial.println(changeCount);
      Serial.print("Timings: ");
      Serial.println(timings5000[0]);
#endif
#ifdef COMP_KW9010
      if (rc == false) {
        rc = receiveProtocolKW9010(changeCount);
      }
#endif
#ifdef COMP_NC_WS
      if (rc == false) {
        rc = receiveProtocolNC_WS(changeCount);
      }
#endif
#ifdef COMP_EUROCHRON
      if (rc == false) {
        rc = receiveProtocolEuroChron(changeCount);
      }
#endif
#ifdef COMP_PT2262
      if (rc == false) {
        rc = receiveProtocolPT2262(changeCount);
      }
#endif
#ifdef COMP_LIFETEC
      if (rc == false) {
        rc = receiveProtocolLIFETEC(changeCount);
      }
#endif
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
  timings5000[changeCount++] = duration;
}

/*
 * decoders with an startbit > 2500
 */
void Startbit_2500(unsigned int duration) {
#define STARTBIT_TIME2         2500
#define STARTBIT_OFFSET2       100

  static unsigned int changeCount;
  static unsigned int repeatCount;
  bool rc = false;

  if (duration > STARTBIT_TIME2 && duration > timings2500[0] - STARTBIT_OFFSET2 && duration < timings2500[0] + STARTBIT_OFFSET2) {
    repeatCount++;
    changeCount--;
    if (repeatCount == 2) {
#ifdef COMP_TX70DTH
      if (rc == false) {
        rc = receiveProtocolTX70DTH(changeCount);
      }
#endif
      if (rc == false) {
        // rc = next decoder;
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
  timings2500[changeCount++] = duration;
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
//    Serial.print(PROGVERS);
//    Serial.println(F(" FHEMduino - compiled at " __DATE__ " " __TIME__));
//    Serial.println(F("V 1.0b1 FHEMduino - compiled at " __DATE__ " " __TIME__));
    Serial.println(F("V " PROGVERS " FHEMduino - compiled at " __DATE__ " " __TIME__));
  }
  // Print free Memory
  else if (cmd.equals("R")) {
    Serial.print(F("R"));
    Serial.println(freeRam());
  }
#ifdef COMP_FA20RF
  // Set FA20RF Repetition
  else if (cmd.startsWith("sdr"))
  {
    char msg[3];
    cmd.substring(3).toCharArray(msg,3);
    FArepetition = atoi(msg);
    Serial.println(cmd);
  }  
  // Switch FA20RF Devices
  else if (cmd.startsWith("sd"))
  {
    digitalWrite(PIN_LED,HIGH);
    char msg[30];
    cmd.substring(2).toCharArray(msg,30);
    sendFA20RF(msg);
    digitalWrite(PIN_LED,LOW);
    Serial.println(msg);
  }
#endif
#ifdef COMP_PT2262
  // Set Intertechno Repetition
  else if (cmd.startsWith("isr"))
  {
    char msg[3];
    cmd.substring(3).toCharArray(msg,3);
    ITrepetition = atoi(msg);
    Serial.println(cmd);
  }  
  // Switch Intertechno Devices
  else if (cmd.startsWith("is"))
  {
    digitalWrite(PIN_LED,HIGH);
    char msg[13];
    cmd.substring(2).toCharArray(msg,13);
    sendPT2262(msg);
    digitalWrite(PIN_LED,LOW);
    Serial.println(cmd);
  }
#endif
  else if (cmd.equals("XQ")) {
    disableReceive();
    Serial.flush();
    Serial.end();
  }
  // Print Available Commands
  else if (cmd.equals("?"))
  {
    Serial.println(F("? Use one of V is isr sd sdr R q"));
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

  bool bitmessage[KW9010_MESSAGELENGTH + 1];
  int bitcount = 0;
  int i = 0;

  if (changeCount < KW9010_MESSAGELENGTH * 2) return false;

  if ((timings5000[0] < KW9010_SYNC - KW9010_GLITCH) || (timings5000[0] > KW9010_SYNC + KW9010_GLITCH)) {
    return false;
  }

  //Serial.println(changeCount);
  for (int i = 2; i < changeCount; i=i+2) {
    if ((timings5000[i] > KW9010_ZERO - KW9010_GLITCH) && (timings5000[i] < KW9010_ZERO + KW9010_GLITCH)) {
      // its a zero
      bitmessage[bitcount] = false;
      bitcount++;
    }
    else if ((timings5000[i] > KW9010_ONE - KW9010_GLITCH) && (timings5000[i] < KW9010_ONE + KW9010_GLITCH)) {
      // its a one
      bitmessage[bitcount] = true;
      bitcount++;
    }
    else {
      return false;
    }
  }

#ifdef DEBUG
    Serial.print("Bit-Stream: ");
    for (i = 0; i < KW9010_MESSAGELENGTH; i++) {
      Serial.print(bitmessage[i]);
    }
    Serial.println();
#endif

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
    calculatedChecksum += (byte)(bitmessage[i*4 + 3] <<3 | bitmessage[i*4 + 2] << 2 | bitmessage[i*4 + 1] << 1 | bitmessage[i*4]);
  }
  calculatedChecksum &= 0xF;

  if (calculatedChecksum == checksum) {
    if (temperature > -500 && temperature < 700) {
      if (humidity > 0 && humidity < 100) {
        char tmp[11];
        sprintf(tmp,"K%02X%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
        message = tmp;
        available = true;
        return true;
      }
    }
  }
  return false;
}
#endif

#ifdef COMP_PT2262
/*
 * PT2262 Stuff
 */
#define RECEIVETOLERANCE       60

bool receiveProtocolPT2262(unsigned int changeCount) {

  message = "IR";
  if (changeCount != 49) {
    return false;
  }
  unsigned long code = 0;
  unsigned long delay = timings5000[0] / 31;
  unsigned long delayTolerance = delay * RECEIVETOLERANCE * 0.01; 

  // 1 3 5 7 9 11 13 15 17 19 21 23 25 27 29 31 33 35 37 39 41 43 45 47
  for (int i = 1; i < changeCount; i=i+2) {
    if (timings5000[i] > delay-delayTolerance && timings5000[i] < delay+delayTolerance && timings5000[i+1] > delay*3-delayTolerance && timings5000[i+1] < delay*3+delayTolerance) {
      code = code << 1;
    }
    else if (timings5000[i] > delay*3-delayTolerance && timings5000[i] < delay*3+delayTolerance && timings5000[i+1] > delay-delayTolerance && timings5000[i+1] < delay+delayTolerance)  { 
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
  unsigned int BaseDur = 350; // Um ggf. die Basiszeit einstellen zu können
  for (int i = 0; i < ITrepetition; i++) {
    unsigned int pos = 0;
    PT2262_sendSync(BaseDur);    
    while (triStateMessage[pos] != '\0') {
      switch(triStateMessage[pos]) {
      case '0':
        PT2262_sendT0(BaseDur);
        break;
      case 'F':
        PT2262_sendTF(BaseDur);
        break;
      case '1':
        PT2262_sendT1(BaseDur);
        break;
      }
      pos++;
    }
  }
}

void PT2262_sendT0(unsigned int BaseDur) {
  PT2262_transmit(1,3,BaseDur);
  PT2262_transmit(1,3,BaseDur);
}

void PT2262_sendT1(unsigned int BaseDur) {
  PT2262_transmit(3,1,BaseDur);
  PT2262_transmit(3,1,BaseDur);
}

void PT2262_sendTF(unsigned int BaseDur) {
  PT2262_transmit(1,3,BaseDur);
  PT2262_transmit(3,1,BaseDur);
}

void PT2262_sendSync(unsigned int BaseDur) {
  PT2262_transmit(1,31,BaseDur);
}

void PT2262_transmit(int nHighPulses, int nLowPulses, unsigned int BaseDur) {
  disableReceive();
  digitalWrite(PIN_SEND, HIGH);
  delayMicroseconds(BaseDur * nHighPulses);
  digitalWrite(PIN_SEND, LOW);
  delayMicroseconds(BaseDur * nLowPulses);
  enableReceive();
}
#endif

#ifdef COMP_NC_WS
/*
 * NC_WS / PEARL NC7159, LogiLink W0002
 */
bool receiveProtocolNC_WS(unsigned int changeCount) {
#define NC_WS_SYNC   9250
#define NC_WS_ONE    3900
#define NC_WS_ZERO   1950
#define NC_WS_GLITCH 100
#define NC_WS_MESSAGELENGTH 36

  if (changeCount < NC_WS_MESSAGELENGTH * 2) {
    return false;
  }
  
  if ((timings5000[0] < NC_WS_SYNC - NC_WS_GLITCH) || (timings5000[0] > NC_WS_SYNC + NC_WS_GLITCH)) {
    return false;
  }

  int i = 0;
  bool bitmessage[NC_WS_MESSAGELENGTH];

  for (i = 0; i < (NC_WS_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings5000[i + 2] > NC_WS_ZERO - NC_WS_GLITCH) && (timings5000[i + 2] < NC_WS_ZERO + NC_WS_GLITCH))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings5000[i + 2] > NC_WS_ONE - NC_WS_GLITCH) && (timings5000[i + 2] < NC_WS_ONE + NC_WS_GLITCH)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }

#ifdef DEBUG
/*  Serial.print("NC_WS: ");
  for (i = 0; i < NC_WS_MESSAGELENGTH; i++) {
    if(i==4) Serial.print(" ");
    if(i==12) Serial.print(" ");
    if(i==13) Serial.print(" ");
    if(i==14) Serial.print(" ");
    if(i==16) Serial.print(" ");
    if(i==17) Serial.print(" ");
    if(i==28) Serial.print(" ");
    if(i==29) Serial.print(" ");
    Serial.print(bitmessage[i]);
  }
  Serial.println(); */

  //                 /--------------------------------- Sensdortype      
  //                /    / ---------------------------- ID, changes after every battery change      
  //               /    /        /--------------------- Battery state 0 == Ok
  //              /    /        /  / ------------------ forced send      
  //             /    /        /  /  / ---------------- Channel (0..2)      
  //            /    /        /  /  /  / -------------- neg Temp: if 1 then temp = temp - 2048
  //           /    /        /  /  /  /   / ----------- Temp
  //          /    /        /  /  /  /   /          /-- unknown
  //         /    /        /  /  /  /   /          /  / Humidity
  //         0101 00101001 0  0  00 0  01000110000 1  1011101
  // Bit     0    4        12 13 14 16 17          28 29    36
#endif

  // Sensor type (Type 5)
  byte unsigned id = 0;
  for (i = 0; i < 4; i++) if (bitmessage[i]) id +=  1 << (3 - i);
  if (id != 5) {
    return false;
  }

  // Sensor ID, will change after ever battery replacement
  id = 0; 
  for (i = 4; i < 12; i++)  if (bitmessage[i]) id +=  1 << (11 - i);

  // Bit 12 : Battery State
  bool battery = !bitmessage[12];

  // Bit 13 : Trigger
  bool forcedSend = bitmessage[13];

  // Bit 14 + 15 = Sensor channel, depends on channel switch: 0 - 2
  byte unsigned channel = bitmessage[15] | bitmessage[14]  << 1;

  // Bit 16 : Temperatur sign (+/-)

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
#endif

#ifdef COMP_EUROCHRON
/*
 * EUROCHRON
 */
bool receiveProtocolEuroChron(unsigned int changeCount) {
#define EuroChron_SYNC   8050
#define EuroChron_ONE    2020
#define EuroChron_ZERO   1010
#define EuroChron_GLITCH  100
#define EuroChron_MESSAGELENGTH 36

  if (changeCount < EuroChron_MESSAGELENGTH * 2) {
    return false;
  }

  int i = 0;
  bool bitmessage[EuroChron_MESSAGELENGTH];

  if ((timings5000[0] < EuroChron_SYNC - EuroChron_GLITCH) && (timings5000[0] > EuroChron_SYNC + EuroChron_GLITCH)) {
    return false;
  }
  
  for (i = 0; i < (EuroChron_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings5000[i + 2] > EuroChron_ZERO - EuroChron_GLITCH) && (timings5000[i + 2] < EuroChron_ZERO + EuroChron_GLITCH))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings5000[i + 2] > EuroChron_ONE - EuroChron_GLITCH) && (timings5000[i + 2] < EuroChron_ONE + EuroChron_GLITCH)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }

#ifdef DEBUG
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

  Serial.print("Bit-Stream: ");
  for (i = 0; i < EuroChron_MESSAGELENGTH; i++) {
    if(i==8) Serial.print(" ");  
    if(i==9) Serial.print(" ");  
    if(i==11) Serial.print(" "); 
    if(i==12) Serial.print(" "); 
    if(i==17) Serial.print(" "); 
    if(i==24) Serial.print(" "); 
    if(i==25) Serial.print(" "); 
    Serial.print(bitmessage[i]);
  }
  Serial.println();
#endif

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
  sprintf(tmp, "C%02x%01d%01d%01d%02d%+04d%02d", id, battery, firstunknown, forcedSend, secunknown, temperature, humidity);
  message = tmp;
  available = true;
  return true;
}
#endif

#ifdef COMP_LIFETEC
/*
 * LIFETEC
 */
bool receiveProtocolLIFETEC(unsigned int changeCount) {
#define LIFETEC_SYNC   8640
#define LIFETEC_ONE    4084
#define LIFETEC_ZERO   2016
#define LIFETEC_GLITCH  460
#define LIFETEC_MESSAGELENGTH 24

  bool bitmessage[24];
  int i = 0;

  if (changeCount < LIFETEC_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings5000[0] < LIFETEC_SYNC - LIFETEC_GLITCH) || (timings5000[0] > LIFETEC_SYNC + LIFETEC_GLITCH)) {
    return false;
  }

  for (i = 0; i < (LIFETEC_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings5000[i + 2] > LIFETEC_ZERO - LIFETEC_GLITCH) && (timings5000[i + 2] < LIFETEC_ZERO + LIFETEC_GLITCH))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings5000[i + 2] > LIFETEC_ONE - LIFETEC_GLITCH) && (timings5000[i + 2] < LIFETEC_ONE + LIFETEC_GLITCH)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }
  
  //                /------------------------------------- Channel, changes after every battery change      
  //               /        / ---------------------------- neg Temp: normal = 000 if 111 then temp = temp - 512      
  //              /        / /---------------------------- Temp      
  //             /        / /       / -------------------- Battery state 1 == Ok      
  //            /        / /       /  /------------------- forced send      
  //           /        / /       /  /  /----------------- filler ?
  //          /        / /       /  /  /  /--------------- TEMP Nachkommastelle
  //         /        / /       /  /  /  /  
  //         11010010 0 0011100 1  0  00 1001
  // Bit     0        8 9       16 17 18 20  24

#ifdef DEBUG
    Serial.print("Bit-Stream: ");
    for (i = 0; i < LIFETEC_MESSAGELENGTH; i++) {
      Serial.print(bitmessage[i]);
    }
    Serial.println();
  
#endif

  // Sensor ID & Channel, will be changed after every battery change
  byte unsigned id = 0;
  for (i = 0; i < 8; i++)  if (bitmessage[i]) id +=  1 << (7 - i);

  // Battery State
  bool battery = bitmessage[16];
  if (battery = 1) {battery = 0;} else if (battery = 0) {battery = 1;}

  // (Propably) Trend
  byte trend = 0; //nicht unterstüzt

  // Trigger
  bool forcedSend = bitmessage[17];
  
  // Temperatur
  int temperature = 0;
  for (i = 9; i < 16; i++) if (bitmessage[i]) temperature +=  1 << (15 - i);
  temperature = temperature * 10;
  for (i = 20; i < 24; i++) if (bitmessage[i]) temperature +=  1 << (23 - i);
  if (bitmessage[8]) temperature = -temperature; // negative Temp

  // Luftfeuchte
  int humidity = 0; //nicht unterstüzt

  char tmp[12];
  sprintf(tmp, "K%02x%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, humidity);
  message = tmp;
  available = true;
  return true;
}
#endif

#ifdef COMP_TX70DTH
/*
 * TX70DTH
 */
bool receiveProtocolTX70DTH(unsigned int changeCount) {
#define TX70DTH_SYNC   4000
#define TX70DTH_ONE    2030
#define TX70DTH_ZERO   1020
#define TX70DTH_GLITCH  250
#define TX70DTH_MESSAGELENGTH 36

  bool bitmessage[TX70DTH_MESSAGELENGTH];
  byte i;
  if (changeCount < TX70DTH_MESSAGELENGTH * 2) {
    return false;
  }
  if ((timings2500[0] < TX70DTH_SYNC - TX70DTH_GLITCH) || (timings2500[0] > TX70DTH_SYNC + TX70DTH_GLITCH)) {
    return false;
  }
  for (i = 0; i < (TX70DTH_MESSAGELENGTH * 2); i = i + 2)
  {
    if ((timings2500[i + 2] > TX70DTH_ZERO - TX70DTH_GLITCH) && (timings2500[i + 2] < TX70DTH_ZERO + TX70DTH_GLITCH))    {
      bitmessage[i >> 1] = false;
    }
    else if ((timings2500[i + 2] > TX70DTH_ONE - TX70DTH_GLITCH) && (timings2500[i + 2] < TX70DTH_ONE + TX70DTH_GLITCH)) {
      bitmessage[i >> 1] = true;
    }
    else {
      return false;
    }
  }

#ifdef DEBUG
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

  Serial.print("Bit-Stream: ");
  if(i==8) Serial.print(" ");  
  if(i==9) Serial.print(" ");  
  if(i==12) Serial.print(" "); 
  if(i==15) Serial.print(" "); 
  if(i==24) Serial.print(" "); 
  if(i==28) Serial.print(" "); 
  for (i = 0; i < TX70DTH_MESSAGELENGTH; i++) {
    Serial.print(bitmessage[i]);
  }
  Serial.println();
#endif

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
  for (i = 16; i < 24; i++) if (bitmessage) temperature +=  1 << (23 - i);
  if (bitmessage[14]) temperature -= 0x200; // negative Temp
  byte feuchte = 0;
  for (i = 29; i < 36; i++) if (bitmessage) feuchte +=  1 << (35 - i);

  // die restlichen 4 Bits sind z.Z unbekannt
  byte rest = 0;
  for (i = 24; i < 27; i++) if (bitmessage) rest +=  1 << (26 - i);

  char tmp[12];
  sprintf(tmp, "K%02x%01d%01d%01d%+04d%02d", id, battery, trend, forcedSend, temperature, feuchte);
  message = tmp;
  available = true;
  return true;
}
#endif


