/*
  Based on wiring.c 
  Part of Arduino - http://www.arduino.cc/

  wiring.c  Copyright (c) 2005-2006 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

#include "wiring_private.h"
#include "myMicros.h"

unsigned long myMicros() {

static volatile unsigned long overFlows = 0;

if(TIFR2 & _BV(TOV2)) {
  sbi(TIFR2,TOV2);
  overFlows++;
}
  uint8_t t;
  uint32_t m;
  m = overFlows;
  t = TCNT2;
  if ((TIFR2 & _BV(TOV2)) && (t < 255))
    m++;

    // When trying to return calculated values when multiplying by the (prescaler/clockCyclesPerMicrosecond)
    // Something doesn't work correctly.  Perhaps the division was happening at the wrong time despite the parentheses
    // and trying explicit datatype conversions.
    // The bit shifting works well, but is limited to just the 2 clock speeds
  #if F_CPU == 16000000
    return ((m<<8) + t) <<3;
  #elif F_CPU == 8000000
    return ((m<<8) + t) <<2;
  #else
    #error CPU speed must be 16 or 8 Mhz
  #endif
}

void myDelay(unsigned long ms)
{
  uint32_t start = myMicros();

  while (ms > 0) {
    yield();
    while ( ms > 0 && (myMicros() - start) >= 1000) {
      ms--;
      start += 1000;
    }
  }
}

void initMyMicros()
{
  uint8_t oldSREG = SREG;
  cli();

#if defined(TCCR2A) && defined(WGM21)
  TCCR2A = 0; //reset from arduino standard configuration
  sbi(TCCR2A, WGM21);
  sbi(TCCR2A, WGM20);
#else
  #error TCCR2A or WGM21 not defined
#endif

  // set timer 2 prescale factor to 256.
#if defined(TCCR2B) && defined(CS21) && defined(CS22)
  // this combination is for the standard 168/328/1280/2560
  TCCR2B = 0; //reset from arduino standard configuration
  sbi(TCCR2B, CS22);
  sbi(TCCR2B, CS21);
#else
  #error Timer 2 prescale factor 256 not set correctly
#endif

  // disable timer 2 interrupt

#if defined(TIMSK2) && defined(TOIE2)
  TIMSK2 = 0; //reset from arduino standard configuration
  //sbi(TIMSK2, TOIE2);
#else
  #error  Timer 2 interrupt not set correctly
#endif


  SREG = oldSREG;

}

