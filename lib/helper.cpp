/*-----------------------------------------------------------------------------------------------
/* Helper functions
-----------------------------------------------------------------------------------------------*/

#include "helper.h"

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

