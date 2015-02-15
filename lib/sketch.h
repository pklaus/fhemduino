/*-----------------------------------------------------------------------------------------------
/* main header file
-----------------------------------------------------------------------------------------------*/

//#define DEBUG           // Compile with Debug informations

#ifndef _sketch_h
  #define _sketch_h
  #if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
  #else
    #include "WProgram.h"
  #endif
#endif

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

#define PIN_LED                13

#ifdef DEBUG
#define BAUDRATE               115200
#else
#define BAUDRATE               9600
#endif

//#define COMP_DCF77      // Compile sketch with DCF-77 Support (currently disableling this is not working, has still to be done)

#define COMP_MAX31850   // Compile sketch with MAX31850 support (thermocouple amplifier using 1-Wire, for k-type thermocouples)

#define COMP_BMP183      // Compile sketch with BMP183 SPI support (high-precision, ultra-low power barometric pressure and temperature sensor)

#define COMP_PT2262     // Compile sketch with PT2262 (IT / ELRO switches)

// #define COMP_DOORBELL   // Compile sketch with door bell support: Tchibo / Heidemann HX Pocket (70283)

#define COMP_FA20RF     // Compile sketch with smoke detector Flamingo FA20RF / ELRO RM150RF

//#define COMP_TEMP_HUM   // General define to compile sketch with temperature / humidity devices
#define COMP_KW9010     // Compile sketch with KW9010 support
#define COMP_NC_WS      // Compile sketch with PEARL NC7159, LogiLink WS0002 support
#define COMP_EUROCHRON  // Compile sketch with EUROCHRON / Tchibo support
//#define COMP_LIFETEC    // Compile sketch with LIFETEC support
#define COMP_TX70DTH    // Compile sketch with TX70DTH (Aldi) support
#define COMP_AURIOL     // Compile sketch with AURIOL (Lidl Version: 09/2013); only temperature

#define COMP_IT_TX      // Compile sketch with Intertechno TX2/3/4 support
#define USE_IT_TX       // Use 14_CUL_TX.pm Module which is already included in fhem. If not defined, the 14_fhemduino_Env module will be used.

#define COMP_OSV2       // Compile sketch with OSV2 Support
//#define COMP_Cresta     // Compile sketch with Cresta Support (currently not implemented, just for future use)
#define USE_OREGON_41   // Use oregon_41 Module which is already included in fhem. If not defined, the 14_fhemduino_oregon module will be used.

// Future enhancement
//#define COMP_OSV3     // Compile sketch with OSV3 Support (currently not implemented, just for future use)
//#define COMP_Kaku     // Compile sketch with Kaku  Support (currently not implemented, just for future use)
//#define COMP_HEZ      // Compile sketch with Homeeasy Support (currently not implemented, just for future use)
//#define COMP_XRF      // Compile sketch with XTF Support (currently not implemented, just for future use)

#ifdef COMP_KW9010      // Compile sketch with KW9010 support
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif
#ifdef COMP_NC_WS       // Compile sketch with PEARL NC7159, LogiLink WS0002 support
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif
#ifdef COMP_EUROCHRON   // Compile sketch with EUROCHRON / Tchibo support
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif
#ifdef COMP_LIFETEC     // Compile sketch with LIFETEC support
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif
#ifdef COMP_TX70DTH     // Compile sketch with TX70DTH (Aldi) support
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif
#ifdef COMP_AURIOL      // Compile sketch with AURIOL (Lidl Version: 09/2013); only temperature
  #define COMP_TEMP_HUM // General define to compile sketch with temperature / humidity devices
#endif

