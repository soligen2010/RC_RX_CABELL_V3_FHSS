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
 
#ifndef __have__TEST_HARNESS_h__
#define __have__TEST_HARNESS_h__

//----------------------------------------------------------------------------------
// Un-comment the line below to compile as a test harness.  In this mode
// there is no servo ouput.  Instead statistics are displayed on a 16x2 LCD.
// This is useful for testing NRF24L01 modules for range/reliability
// and for testing antennas.

//#define TEST_HARNESS

// The LCD displays 6 numbers.
//
// Line 1:  Packet success rate (percent); Number of missed packets in a row; The number of time the secondary reciever recovers a packet the primary reciever missed
//
// Line 2:  The number of good packets recieved; The number of missed or bad packets; The number of packets recieved with strengh below -64db
//
//----------------------------------------------------------------------------------

#ifdef TEST_HARNESS

#include "Arduino.h"

#define PACKET_DISPLAY_INTERVAL  1000    // The number of expected packets between display updates. 1000 is 3 seconds non telemetry, 3.5 with telemetry

class TestHarness {

  public:
      TestHarness ( );
                                     
      void init();
      void hit();
      void miss();
      void failSafe();
      void reSync();
      void secondaryHit();      
      void badPacket();      

     
  private:
      void display(int LCD_Command);
      void packetProcess();
      void resetCounters();
                 
      int hitCount = 0;
      int missCount = 0;
      int sequentialMissCount = 0;
      int holdSequentialMissCount = 0;
      int secondaryHitCount = 0;
      int badPacketCount = 0;
      int packetCount = 0;

      bool firstPacketHit = false;
      bool firstDisplay = false;

      int displayHitCount = 0;
      int displayMissCount = 0;
      int displaySequentialMissCount = 0;
      int displayHoldSequentialMissCount = 0;
      int displaySecondaryHitCount = 0;
      int displayBadPacketCount = 0;
      int displayPacketCount = 0;
      float displayPacketRate = 0.0;

};


#endif
#endif

