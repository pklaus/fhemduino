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
// 2014-06-24 - Added send / activate support for smoke detectors FA20RF / RM150RF / Brennenstuhl BR 102-F / (KD101 not verified)
// 2014-06-24 - Integrated mick6300 developments LIFETEC support
// 2014-06-24 - Integrated mick6300 developments TX70DTH (Aldi) support
// 2014-07-04 - Integrated Intertechno TX2/3/4 support, see CUL_TX and http://www.f6fbb.org/domo/sensors/tx3_th.php
// 2014-07-15 - Changed Stop-Bit interpretaion for smoke detectors FA20RF / RM150RF / Brennenstuhl BR 102-F / KD101
// 2014-07-16 - Return value for Lifetec changed to distribute to 14_FEHMduino_Lifetec.pm
// 2014-07-16 - Added Tchibo TCM234759 door bell
// 2014-07-17 - Added  * Heidemann HX Pocket (70283). May work also with other Heidemann HX series door bells
// 2014-07-31 - Fixed bug in KW9010 / crcValid not needed any more
// 2014-08-05 - Added temperature sensor AURIOL (Lidl Version: 09/2013)
// 2014-08-06 - Implemented uptime
// 2014-08-08 - Started outsourcing of devices in modules

// --- Configuration ---------------------------------------------------------
#define PROGNAME               "FHEMduino"
#define PROGVERS               "2.3"

/*-----------------------------------------------------------------------------------------------
/* Please set defines in sketch.h
-----------------------------------------------------------------------------------------------*/
#include "sketch.h"

/*
 * Modified code to fit info fhemduino - Sidey
 * Oregon V2 decoder modfied - Olivier Lebrun
 * Oregon V2 decoder added - Dominique Pierre
 * New code to decode OOK signals from weather sensors, etc.
 * 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
 *
*/

/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_TEMP_HUM
  #include "temphum.h"
#endif

/*-----------------------------------------------------------------------------------------------
/* Devices with temperatur / humidity functionality => Intertechno TX2/3/4
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_IT_TX      // Compile sketch with Intertechno TX2/3/4 support
  #include "it_tx.h"
#endif

/*-----------------------------------------------------------------------------------------------
/* Devices with sending / receiving functionality => PT2262
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_PT2262
  #include "PT2262.h"
#endif

/*-----------------------------------------------------------------------------------------------
/* door bell support: Tchibo / Heidemann HX Pocket (70283)
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_DOORBELL
  #include "doorbell.h"
#endif

/*-----------------------------------------------------------------------------------------------
/* Smoke dector FA20RF
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_FA20RF
  #include "FA20RF.h"
#endif

/*-----------------------------------------------------------------------------------------------
/* Helper functions
-----------------------------------------------------------------------------------------------*/
#include "helper.h"

/*-----------------------------------------------------------------------------------------------
/* include oregon class
-----------------------------------------------------------------------------------------------*/
#include "oregon.h"

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

/*-----------------------------------------------------------------------------------------------
/* MAX31850 1-Wire support
-----------------------------------------------------------------------------------------------*/
#ifdef COMP_MAX31850
#include "OneWire.h"
#include "DallasTemperature.h"

#define ONE_WIRE_BUS 4 // Data pin of MAX31850 (use a level shifter to 3V3!)

#define TEMPERATURE_PRECISION 9
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int max31850_num_devices; // Number of temperature devices found
int max31850_current_device = 0;
int max31850_tries = 0;
int max31850_max_tries = 10;
DeviceAddress max31850_temp_device_address;
char max31850_msg[64];

static unsigned long max31850_last_time = 0;
const unsigned long max31850_interval = 2000;

bool handle_max31850() {
  disableReceive();
  float tempC;
  if (sensors.isConversionAvailable(max31850_temp_device_address)) {
    while (max31850_tries < max31850_max_tries) {
      max31850_tries++;
      tempC = sensors.getTempC(max31850_temp_device_address);
      if (tempC < -270.0 || tempC > 1768.0 || tempC != tempC) continue;
      else break;
    }
    long int tempCint;
    if (tempC >= -270.0 && tempC <= 1768.0 && tempC == tempC) {
      tempCint = (long int) (tempC * 100.0);
    } else {
      // we failed to read this sensor ->  let's send an error message
      tempCint = (long int) 999999;
    }
    // Output
    sprintf(max31850_msg,"y %02x%02x%02x%02x%02x%02x%02x%02x %+07ld", max31850_temp_device_address[0], max31850_temp_device_address[1], max31850_temp_device_address[2], max31850_temp_device_address[3], max31850_temp_device_address[4], max31850_temp_device_address[5], max31850_temp_device_address[6], max31850_temp_device_address[7], tempCint);
    message = max31850_msg;
    available = true;
    // Move on to next sensor
    max31850_tries = 0;
    max31850_current_device++;
    max31850_current_device %= max31850_num_devices;
    sensors.getAddress(max31850_temp_device_address, max31850_current_device);
    sensors.requestTemperatures();
    return true;
  } else {
    sensors.requestTemperatures();
    if (max31850_tries >= max31850_max_tries) {
      max31850_current_device++;
      max31850_current_device %= max31850_num_devices;
      sensors.requestTemperatures();
      max31850_tries = 0;
    } else max31850_tries++;
  }
  enableReceive();
}

#endif

/*-----------------------------------------------------------------------------------------------
/* DCF77 stuff
-----------------------------------------------------------------------------------------------*/
/*
 * DCF77_SerialTimeOutput
 * Ralf Bohnen, 2013
 * This example code is in the public domain.
 */
#ifdef COMP_DCF77
#include <Time.h>        // Unterstuetzung für Datum/Zeit-Funktionen
#include <DCF77.h>
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

/*-----------------------------------------------------------------------------------------------
/* Globals for message handling
-----------------------------------------------------------------------------------------------*/
String cmdstring;
volatile bool available = false;
String message = "";

/*-----------------------------------------------------------------------------------------------
/* Globals for bitstream handling
-----------------------------------------------------------------------------------------------*/
#define MAX_CHANGES            90
unsigned int timings[MAX_CHANGES];
unsigned int timings2500[MAX_CHANGES];

/*-----------------------------------------------------------------------------------------------
/* Initializing aof Arduino
-----------------------------------------------------------------------------------------------*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
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

#ifdef COMP_MAX31850
  sensors.begin();
  sensors.setWaitForConversion(false);
  max31850_num_devices = sensors.getDeviceCount();
  for(int i=0;i<max31850_num_devices; i++) {
    if(sensors.getAddress(max31850_temp_device_address, i)) {
      sensors.setResolution(max31850_temp_device_address, TEMPERATURE_PRECISION);
    }
  }
  if (max31850_num_devices > 0) sensors.getAddress(max31850_temp_device_address, 0);
#endif

  enableReceive();
}

/*-----------------------------------------------------------------------------------------------
/* Main loop
-----------------------------------------------------------------------------------------------*/
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

#ifdef COMP_MAX31850
  if (millis() - max31850_last_time >= max31850_interval) {
    max31850_last_time = max31850_last_time + max31850_interval;
    handle_max31850();
  }
#endif

//serialEvent does not work on ATmega32U4 devices like the Leonardo, so we do the handling ourselves
#if defined(__AVR_ATmega32U4__)
  if (Serial.available()) {
    serialEvent();
  }
#endif
}

/*-----------------------------------------------------------------------------------------------
/* Interrupt system
-----------------------------------------------------------------------------------------------*/

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

  decoders(duration);
  decoders2500(duration);

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
    const byte* data = cres.getData(len) + 5;
    char tmp[36]="";
    uint8_t tmp_len = 0;
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

/*-----------------------------------------------------------------------------------------------
/* Generation bitstreams with sync bits greater than 5000 us
-----------------------------------------------------------------------------------------------*/
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
#ifdef COMP_DOORBELL
      if (rc == false) {
        rc = receiveProtocolTCM(changeCount);
      }
      if (rc == false) {
        rc = receiveProtocolHX(changeCount);
      }
#endif
#ifdef COMP_AURIOL
      if (rc == false) {
        rc = receiveProtocolAURIOL(changeCount);
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
  timings[changeCount++] = duration;
}

/*-----------------------------------------------------------------------------------------------
/* Generation bitstreams with sync bits greater than 2500 us and less than 5000 us
-----------------------------------------------------------------------------------------------*/
void decoders2500(unsigned int duration) {
#define LOW_STARTBIT_TIME  2500
#define HIGH_STARTBIT_TIME 2500
#define STARTBIT_OFFSET 200

  static unsigned int changeCount;
  static unsigned int repeatCount;
  bool rc = false;

  if ((duration > LOW_STARTBIT_TIME && duration < HIGH_STARTBIT_TIME) && duration > timings2500[0] - STARTBIT_OFFSET && duration < timings2500[0] + STARTBIT_OFFSET) {
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
  else if (duration > STARTBIT_TIME) {
    changeCount = 0;
  }

  if (changeCount >= MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }
  timings2500[changeCount++] = duration;
}

/*-----------------------------------------------------------------------------------------------
/* Serial Command Handling
-----------------------------------------------------------------------------------------------*/
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
#ifdef COMP_FA20RF
  // Set FA20RF Repetition
  else if (cmd.startsWith("f"))
  {
     FA20RF_CMDs(cmd);
  }
#endif
#ifdef COMP_PT2262
  // Intertechno commands
  else if (cmd.startsWith("i")) {
    PT2262_CMDs(cmd);
  }
#endif
#ifdef COMP_DOORBELL
  // Tchibo TCM commands
  else if (cmd.startsWith("d")) {
    TCM_CMDs(cmd);
  }
  // Heidemann commands
  else if (cmd.startsWith("h")) {
    HeideTX_CMDs(cmd);
  }
#endif
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
    Serial.println(F("? Use one of V i f d h t R q"));
  }
  cmdstring = "";
}

