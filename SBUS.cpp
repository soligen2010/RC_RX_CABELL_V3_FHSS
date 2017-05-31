/* 
 To use this software, you must adhere to the license terms described below, and assume all responsibility for the use
 of the software.  The user is responsible for all consequences or damage that may result from using this software.
 The user is responsible for ensuring that the hardware used to run this software complies with local regulations and that 
 any radio signal generated from use of this software is legal for that user to generate.  The author(s) of this software 
 assume no liability whatsoever.  The author(s) of this software is not responsible for legal or civil consequences of 
 using this software, including, but not limited to, any damages cause by lost control of a vehicle using this software.  
 If this software is copied or modified, this disclaimer must accompany all copies.
 
 This PPM algorithm originated from:
             https://code.google.com/archive/p/generate-ppm-signal/
 under GNU GPL 2.  Second version dated Mar 14, 2013.
 Modifications and additions released here under GPL.
 */

/*
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
 
#include "RX.h"
#include "Arduino.h"
#include "SBUS.h"

bool sbusEnabledFlag = false;
volatile uint8_t sbusPacket[25];

//------------------------------------------------------------------------------------------------------------------------
void sbusSetup(){  
  noInterrupts();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sbusPacket[SBUS_START_BYTE] = 0xF0;
  sbusPacket[SBUS_FLAG_BYTE]  = 0x00;
  sbusPacket[SBUS_END_BYTE]   = 0x00;
  sbusEnabledFlag = true;
  interrupts();
}

bool sbusEnabled() {
  return sbusEnabledFlag;
}

//------------------------------------------------------------------------------------------------------------------------
void sbusDisable(){    
  noInterrupts();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  TIMSK1 &= ~(1<<OCIE1A);   // Disable Interrupt Counter 1, output compare A (TIMER1_CMPA_vect)
  interrupts();
  sbusEnabledFlag = false;
}

//------------------------------------------------------------------------------------------------------------------------
void setSbusOutputChannelValue(uint8_t channel, int value) {

  uint8_t firstBit = 8 + (constrain(channel,0,15) * 11);  // Start byte plus 11 bits per channel. 16 channels
  uint8_t byteIndex = firstBit / 8;
  uint8_t bitIndex = 7 - (firstBit % 8);
  uint16_t adjustedValue = value - 477;

  noInterrupts();                       //Turn off interrupts so that SBUS_ISR does not run while a value is being updated
  for (uint8_t x = 0; x < 11; x++) {
    if (adjustedValue & 0x0001) {
      sbusPacket[byteIndex] |= _BV(bitIndex);
    } else {
      sbusPacket[byteIndex] &= ~_BV(bitIndex);      
    }
    adjustedValue >>=  1; 
    if (bitIndex == 0) {
      bitIndex = 7;
      byteIndex++;
    } else{
      bitIndex--;      
    }    
  }
  interrupts();
}

//------------------------------------------------------------------------------------------------------------------------
void sbusSetFailsafe(bool value) {
  noInterrupts();                       //Turn off interrupts so that SBUS_ISR does not run while a value is being updated
  if (value) {
    sbusPacket[SBUS_FLAG_BYTE] |= SBUS_FAILSAFE_MASK;
  } else {
    sbusPacket[SBUS_FLAG_BYTE] &= ~SBUS_FAILSAFE_MASK;    
  }
  interrupts();
}

//------------------------------------------------------------------------------------------------------------------------
void sbusSetFrameLost(bool value) {
  noInterrupts();                       //Turn off interrupts so that SBUS_ISR does not run while a value is being updated
  if (value) {
    sbusPacket[SBUS_FLAG_BYTE] |= SBUS_FRAME_LOST_MASK;
  } else {
    sbusPacket[SBUS_FLAG_BYTE] &= ~SBUS_FRAME_LOST_MASK;    
  }
  interrupts();
}

//------------------------------------------------------------------------------------------------------------------------
void SBUS_ISR() {  
  // Copy the sbusPacket to the serial output buffer and send it (8E2 at 100000 baud 

}

