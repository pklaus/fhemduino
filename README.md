fhemduino
=========


How to compile with Arduino IDE unter Microsoft Windows:


1. Sync the repository from github or download the complete archive via zip
2. Extract or copy all files from src and lib the files to a directory called fhemduino
3. rename sketch.ino into fhemduino.ino. The name of the sketch must be the same as the directory where it resides.
4. Look at the sketch.h file, to enable or disable features of the fhemduino.
5. Open fhemduino.ino in the IDE and just compile it.


How to enable / disable features?
=========

Features can be toggelt to on with removing the slashes in front of the switch.
Example:
To compile the sketch with support for KW9010 sensor:
#define COMP_KW9010     // Compile sketch with KW9010 support

To compile the sketch without support for KW9010 sensor:
//#define COMP_KW9010     // Compile sketch with KW9010 support



The following features can be enabled / disabled during compile time. Others may work or do not work.


#define COMP_DCF77      // Compile sketch with DCF-77 Support (currently disableling this is not working, has still to be done)
#define COMP_PT2262     // Compile sketch with PT2262 (IT / ELRO switches)
#define COMP_DOORBELL   // Compile sketch with door bell support: Tchibo / Heidemann HX Pocket (70283)
#define COMP_FA20RF     // Compile sketch with smoke detector Flamingo FA20RF / ELRO RM150RF
#define COMP_TEMP_HUM   // General define to compile sketch with temperature / humidity devices
#define COMP_KW9010     // Compile sketch with KW9010 support
#define COMP_NC_WS      // Compile sketch with PEARL NC7159, LogiLink WS0002 support
#define COMP_EUROCHRON  // Compile sketch with EUROCHRON / Tchibo support
#define COMP_LIFETEC    // Compile sketch with LIFETEC support
#define COMP_TX70DTH    // Compile sketch with TX70DTH (Aldi) support
#define COMP_AURIOL     // Compile sketch with AURIOL (Lidl Version: 09/2013); only temperature
#define COMP_IT_TX      // Compile sketch with Intertechno TX2/3/4 support
#define USE_IT_TX       // Use 14_CUL_TX.pm Module which is already included in fhem. If not defined, the 14_fhemduino_Env module will be used.


Do not disable #define COMP_OSV2 this will result in an error.

