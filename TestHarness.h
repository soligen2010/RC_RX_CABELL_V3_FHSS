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
// The LCD displays 5 numbers.
//
// Line 1:  Packet success rate (percent); Number of missed packets in a row; The number of time the secondary receiver recovers a packet the primary receiver missed
//
// Line 2:  The number of good packets received; The number of missed or bad packets
//
// Un-comment the line below to compile as a test harness.  In this mode
// there is no servo output.  Instead statistics are displayed on a 16x2 LCD.
// This is useful for testing NRF24L01 modules for range/reliability
// and for testing antennas.

//#define TEST_HARNESS
//#define USE_I2C_LCD

// For I2C LCD the pins for the I2C backpack port extended need to be specified along with the I2C address
// There are various version on the market, so these may need to be changed which version is being used
// some version are documented at https://arduino-info.wikispaces.com/LCD-Blue-I2C
// I2C requires the NewLiquidCrystal library to be installed.  Get it at https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/
#ifdef USE_I2C_LCD
  #define LCD_I2C_ADDR     0x3F
  //#define LCD_I2C_ADDR     0x27
  #define I2C_EN_PIN              2
  #define I2C_RW_PIN              1
  #define I2C_RS_PIN              0
  #define I2C_D4_PIN              4
  #define I2C_D5_PIN              5
  #define I2C_D6_PIN              6
  #define I2C_D7_PIN              7
  #define I2C_BL_PIN              3   //Back light
  #define I2C_BACKLIGHT_POLARITY  POSITIVE
#endif

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

