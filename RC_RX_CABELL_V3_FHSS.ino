/*
 Copyright 2017 by Dennis Cabell
 KE8FZX
 
 To use this software, you must adhere to the license terms described below, and assume all responsibility for the use
 of the software.  The user is responsible for all consequences or damage that may result from using this software.
 The user is responsible for ensuring that the hardware used to run this software complies with local regulations and that 
 any radio signal generated from use of this software is legal for that user to generate.  The author(s) of this software 
 assume no liability whatsoever.  The author(s) of this software is not responsible for legal or civil consequences of 
 using this software, including, but not limited to, any damages cause by lost control of a vehicle using this software.  
 If this software is copied or modified, this disclaimer must accompany all copies.
 
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 RC_RX_CABELL_V3_FHSS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with RC_RX_CABELL_V3_FHSS.  If not, see <http://www.gnu.org/licenses/>.
 */
 
  /* Library dependencies:
   *  
   *  Aaduino Core, SPI, and EEPROM
   *  http://tmrh20.github.io/RF24 for the NRF24L01 (Use trhe soligen2010 fork if any changes to this library casues issues)
   *  
   *  Arduino Servo was modified and is included with this source.  It was changed to not directly define the Timer 1 ISR
   */
   
#include "RX.h"
#include "Pins.h"

//--------------------------------------------------------------------------------------------------------------------------
void setup(void) {
  Serial.begin(74880);
  Serial.println(); Serial.println(F("Initializing"));
  
  pinMode (BIND_BUTTON_PIN,INPUT_PULLUP);  // used for bind plug or button

  pinMode (LED_PIN, OUTPUT);     //status LED
  digitalWrite(LED_PIN, LOW);
 
  setupReciever();

  Serial.println(F("Starting main loop ")); 
}

//--------------------------------------------------------------------------------------------------------------------------
void loop() {  
  if (getPacket()) 
    outputChannels();
}






