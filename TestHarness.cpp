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

#include "TestHarness.h" 
#include "Pins.h" 

#ifdef TEST_HARNESS

#include "Arduino.h"

#ifdef USE_I2C_LCD
  #include <LiquidCrystal_I2C.h>

  LiquidCrystal_I2C lcd(LCD_I2C_ADDR,   // Set the LCD I2C address
                        I2C_EN_PIN,
                        I2C_RW_PIN,
                        I2C_RS_PIN,
                        I2C_D4_PIN,
                        I2C_D5_PIN,
                        I2C_D6_PIN,
                        I2C_D7_PIN,
                        I2C_BL_PIN,
                        I2C_BACKLIGHT_POLARITY);  

#else
  #include <LiquidCrystal.h>

  LiquidCrystal lcd(    RS_PIN,
                        EN_PIN,                      
                        D4_PIN,
                        D5_PIN,
                        D6_PIN,
                        D7_PIN);  
#endif
                        
//--------------------------------------------------------------------------------------------------------------------------
TestHarness::TestHarness ()
{
}

//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::init() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);lcd.print(F("FHSS PKT Rate %"));
  lcd.setCursor(0, 1);lcd.print(F("Searching ..."));
}

//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::hit() {
  if (!firstPacketHit) resetCounters();
  firstPacketHit = true;

  hitCount++;
  sequentialMissCount = 0;

  packetProcess();
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::miss() {
  missCount++;
  sequentialMissCount++;
  if (sequentialMissCount  > holdSequentialMissCount) holdSequentialMissCount = sequentialMissCount;

  packetProcess();
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::secondaryHit() {
  secondaryHitCount++;
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::badPacket() {
  badPacketCount++;
  hit();
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::failSafe() {
  if (firstDisplay) {
    lcd.clear();
    lcd.setCursor(0, 0);lcd.print(F("Fail-safe"));
  }
  resetCounters();
  displayPacketCount = 0;
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::reSync() {
  if (firstDisplay) {
    lcd.clear();
    lcd.setCursor(0, 0);lcd.print(F("Re-sync"));
    firstDisplay = false;
  }
  resetCounters();
  displayPacketCount = 0;
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::packetProcess() {

  if (!firstPacketHit) {
    resetCounters();
  } else {
    packetCount++;
    if (packetCount >= PACKET_DISPLAY_INTERVAL) {
      displayHitCount = hitCount;
      displayMissCount = missCount;
      displaySequentialMissCount = holdSequentialMissCount;
      displayHoldSequentialMissCount = holdSequentialMissCount;
      displaySecondaryHitCount = secondaryHitCount;
      displayBadPacketCount = badPacketCount;
      displayPacketCount = packetCount;
      displayPacketRate = (float)(displayHitCount - displayBadPacketCount)*100.0/(float)(displayHitCount+displayMissCount);

      resetCounters();
    }
    if (firstPacketHit && displayPacketCount > 0) {
      display(packetCount);
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::resetCounters() {
  hitCount = 0;
  missCount = 0;
  sequentialMissCount = 0;
  holdSequentialMissCount = 0;
  secondaryHitCount = 0;
  badPacketCount = 0;
  packetCount = 0;
  firstPacketHit = false;
}

//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::display(int LCD_Command) {
  // updateing the lcd is slow, so change display gradually, one character at a time

static char line[18];

if ( LCD_Command  == 1)  sprintf(line,"%-7i%-4i%5i",(int)displayPacketRate,displaySequentialMissCount,displaySecondaryHitCount);

if ( LCD_Command  == 3)  lcd.setCursor(0, 0);
if ( LCD_Command  >= 5  && LCD_Command < 5+16)  lcd.write(line[LCD_Command-5]);


if ( LCD_Command  == 22)  sprintf(line,"%-7i%-9i",displayHitCount,displayMissCount);

if ( LCD_Command  == 24)  lcd.setCursor(0, 1);
if ( LCD_Command  >= 26 && LCD_Command < 26+16)  lcd.write(line[LCD_Command-26]);

firstDisplay = true;
}


#endif
