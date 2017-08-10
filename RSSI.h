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
 
#ifndef __have__RSSI_h__
#define __have__RSSI_h__

#include "Arduino.h"

#define RSSI_CALC_INTERVAL  152    // The number of expected packets between calculating the base RSSI.  Approximately 1/2 second
#define TELEMETRY_RSSI_MIN_VALUE        0                     // The lowest possible RSSI value = zero packet rate
#define TELEMETRY_RSSI_MAX_VALUE        100                   // The highest possible RSSI value = 100% packet rate
#define RSSI_MISS_ADJUSTMENT            5                     // Each consecutive miss deducts this from the packet rate
#define RSSI_MISS_ADJUSTMENT_RECOVERY   1                     // The rate at which a hit removes the miss adjustment

class RSSI {

  public:
      RSSI ( );
                                     
      void hit();
      void miss();
      void secondaryHit();      
      void badPacket(); 
      uint8_t getRSSI();     
     
  private:
      void packetProcess();
      void resetCounters();
                 
      int hitCount = 0;
      int missCount = 0;
      int secondaryHitCount = 0;
      int badPacketCount = 0;
      int packetCount = 0;

      int packetRate = 0.0;
      int8_t sequentialMissTrack = 0;
};


#endif


