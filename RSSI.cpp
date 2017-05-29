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

#include "RSSI.h" 
#include "Arduino.h" 
                        
//--------------------------------------------------------------------------------------------------------------------------
RSSI::RSSI ()
{
}

//--------------------------------------------------------------------------------------------------------------------------
void RSSI::hit() {
  hitCount++;
  sequentialMissTrack = constrain(sequentialMissTrack - RSSI_MISS_ADJUSTMENT_RECOVERY,TELEMETRY_RSSI_MIN_VALUE,TELEMETRY_RSSI_MAX_VALUE);
  packetProcess();
}


//--------------------------------------------------------------------------------------------------------------------------
void RSSI::miss() {
  missCount++;
  sequentialMissTrack = constrain(sequentialMissTrack + RSSI_MISS_ADJUSTMENT,TELEMETRY_RSSI_MIN_VALUE,TELEMETRY_RSSI_MAX_VALUE);
  packetProcess();
}


//--------------------------------------------------------------------------------------------------------------------------
void RSSI::secondaryHit() {
  secondaryHitCount++;
}


//--------------------------------------------------------------------------------------------------------------------------
void RSSI::badPacket() {
  // bad packets generally don't happen, but it will also count as a hit since the error was likely introduced on the SPI bus since it passed the NRF24L01 internal checksum
  badPacketCount++;
  hit();
}


//--------------------------------------------------------------------------------------------------------------------------
void RSSI::packetProcess() {
  packetCount++;
  if (packetCount >= RSSI_CALC_INTERVAL) {
    packetRate = constrain((int)((float)(hitCount - badPacketCount)*100.0/(float)(hitCount+missCount)),TELEMETRY_RSSI_MIN_VALUE,TELEMETRY_RSSI_MAX_VALUE);
    resetCounters();
  }
}


//--------------------------------------------------------------------------------------------------------------------------
void RSSI::resetCounters() {
  hitCount = 0;
  missCount = 0;
  secondaryHitCount = 0;
  badPacketCount = 0;
  packetCount = 0;
}


//--------------------------------------------------------------------------------------------------------------------------
uint8_t RSSI::getRSSI() {
 return constrain(packetRate - sequentialMissTrack,TELEMETRY_RSSI_MIN_VALUE,TELEMETRY_RSSI_MAX_VALUE);
}

