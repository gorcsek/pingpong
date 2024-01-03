#include "DigiKeyboard.h"
#include<TinyWireS.h>

void setup() {
  TinyWireS.begin(4);
}
//
void loop() {
  if (TinyWireS.available()){           // got I2C input!
    int byteRcvd = TinyWireS.receive();     // get the byte from master
    DigiKeyboard.print(byteRcvd);
  }  
}

223334111222223