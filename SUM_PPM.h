/*
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
 
#ifndef __have__SUM_PPM_h__
#define __have__SUM_PPM_h__

#include "pins.h"

////////////////////// CONFIGURATION ///////////////////////////////
#define PPM_FrLen             22500                      //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_MaxChannels       8                          //The maximum number of channels that can be sent in a frame
#define PPM_PulseLen_us       300                        //set the pulse length
#define onState               1                          //set polarity of the pulses: 1 is positive, 0 is negative
#define TICKS_PER_US          2                          //based on timer settings, the number of ticks per microsecond
///////////////////// CALIBRATION //////////////////////////////////
#define MICROSECOND_RANGE_EXPANSION       .02            // If the low to high range is not 1000, then adjust this to get the difference to 1000
#define MICROSECOND_RANGE_OFFSET          -2             // If the output range is not centered at 1500, use this to offset.  Adjust after MICROSECOND_RANGE_EXPANSION
////////////////////////////////////////////////////////////////////
#define PPM_PulseLen_ticks  (PPM_PulseLen_us * TICKS_PER_US)      //set the pulse length
#define PPM_FrLen_ticks     (PPM_FrLen * TICKS_PER_US)            //set the PPM frame length in microseconds (1ms = 1000µs)

#if onState == 1
  #define  PPM_PIN_ON  PPM_OUTPUT_SET_HIGH
  #define  PPM_PIN_OFF PPM_OUTPUT_SET_LOW
#else
  #define  PPM_PIN_ON  PPM_OUTPUT_SET_LOW
  #define  PPM_PIN_OFF PPM_OUTPUT_SET_HIGH
#endif

void ppmSetup(uint8_t pin, uint8_t channelCount);
void ppmDisable();
bool PPMEnabled();
void setPPMOutputChannelValue(uint8_t channel, uint16_t value);
void SUM_PPM_ISR();

#endif
