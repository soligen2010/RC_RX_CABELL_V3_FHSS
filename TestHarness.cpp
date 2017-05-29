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

#include <LiquidCrystal.h>
#include "Arduino.h"

  LiquidCrystal lcd(    RS_PIN,
                        EN_PIN,                      
                        D4_PIN,
                        D5_PIN,
                        D6_PIN,
                        D7_PIN);  
                        
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
}


//--------------------------------------------------------------------------------------------------------------------------
void TestHarness::reSync() {
  if (firstDisplay) {
    lcd.clear();
    lcd.setCursor(0, 0);lcd.print(F("Re-sync"));
  }
  resetCounters();
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
  // updateing the lcd is slow, so change display gradually

  if ( LCD_Command  == 1)  lcd.setCursor(0, 0);
  if ( LCD_Command  == 3)  lcd.print("       ");
  if ( LCD_Command  == 5)  lcd.setCursor(0, 0);
  if ( LCD_Command  == 7)  lcd.print(displayPacketRate,1);
  if ( LCD_Command  == 9)  lcd.setCursor(7, 0);
  if ( LCD_Command  == 11)  lcd.print("     ");
  if ( LCD_Command  == 13)  lcd.setCursor(7, 0);
  if ( LCD_Command  == 15)  lcd.print(displaySequentialMissCount);
  if ( LCD_Command  == 17)  lcd.setCursor(0, 1);
  if ( LCD_Command  == 19)  lcd.print("       ");
  if ( LCD_Command  == 21)  lcd.setCursor(0, 1);
  if ( LCD_Command  == 23)  lcd.print(displayHitCount);
  if ( LCD_Command  == 25)  lcd.setCursor(7, 1);
  if ( LCD_Command  == 27)  lcd.print("      ");
  if ( LCD_Command  == 29)  lcd.setCursor(7, 1);
  if ( LCD_Command  == 31)  lcd.print(displayMissCount);
  if ( LCD_Command  == 41)  lcd.setCursor(12, 0);
  if ( LCD_Command  == 43)  lcd.print("    ");
  if ( LCD_Command  == 45)  lcd.setCursor(12, 0);
  if ( LCD_Command  == 47)  lcd.print(displaySecondaryHitCount);

  firstDisplay = true;
}


#endif
