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
 
#ifndef __have__SBUS_h__
#define __have__SBUS_h__

#define SBUS_START_BYTE  0
#define SBUS_END_BYTE    24
#define SBUS_FLAG_BYTE   23

#define SBUS_FAILSAFE_MASK    0x10
#define SBUS_FRAME_LOST_MASK  0x20

void sbusSetup();
void sbusDisable();
bool sbusEnabled();
void setSbusOutputChannelValue(uint8_t channel, int value);
void SBUS_ISR();
void sbusSetFailsafe(bool value);
void sbusSetFrameLost(bool value);

#endif
