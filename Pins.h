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
 
#ifndef __have__RC_RX_PINS_h__
#define __have__RC_RX_PINS_h__

#include "RX.h"

#define RADIO1_CSN_PIN            14     // AKA A0
#define RADIO2_CSN_PIN            10     // Second radio for diversity.  Some older hardware used this pin as CE
#define RADIO1_CE_PIN             RADIO1_CSN_PIN
#define RADIO2_CE_PIN             RADIO2_CSN_PIN

#define SPI_MOSI                  11
#define SPI_MISO                  12
#define SPI_SCLK                  13

#define ROLL_PIN                  2    
#define PITCH_PIN                 3    
#define THROTTLE_PIN              4    
#define YAW_PIN                   5    
#define AUX1_PIN                  6   
#define AUX2_PIN                  7    
#define AUX3_PIN                  8    
#define AUX4_PIN                  9    

#define PPM_OUTPUT_PIN            2
#define PPM_OUTPUT_PIN_bit        2           //2 = PD2
#define PPM_OUTPUT_port           PORTD
#define PPM_OUTPUT_ipr            PIND
#define PPM_OUTPUT_ddr            DDRD
#define PPM_OUTPUT_PIN_MASK       _BV(PPM_OUTPUT_PIN_bit)
#define PPM_OUTPUT_SET_INPUT      PPM_OUTPUT_ddr &= ~PPM_OUTPUT_PIN_MASK
#define PPM_OUTPUT_SET_OUTPUT     PPM_OUTPUT_ddr |=  PPM_OUTPUT_PIN_MASK
#define PPM_OUTPUT_SET_PULLUP     PPM_OUTPUT_port |= PPM_OUTPUT_PIN_MASK
#define PPM_OUTPUT_SET_HIGH       PPM_OUTPUT_port |= PPM_OUTPUT_PIN_MASK
#define PPM_OUTPUT_SET_LOW        PPM_OUTPUT_port &= ~PPM_OUTPUT_PIN_MASK

#define BIND_BUTTON_PIN           A3
#define LED_PIN                   A1

#define TELEMETRY_ANALOG_INPUT_1  6   // Pin A6
#define TELEMETRY_ANALOG_INPUT_2  7   // Pin A7

#ifdef TEST_HARNESS
  // Pins for test harness LCD display
  #define D4_PIN     9
  #define D5_PIN     8
  #define D6_PIN     7
  #define D7_PIN     6
  #define RS_PIN     5
  #define EN_PIN     4
#endif

// configure A2 for radio IRQ 
#define RADIO_IRQ_PIN          A2 
#define RADIO_IRQ_PIN_bit      2           //A2 = PC2
#define RADIO_IRQ_port         PORTC
#define RADIO_IRQ_ipr          PINC
#define RADIO_IRQ_ddr          DDRC
#define RADIO_IRQ_PIN_MASK     _BV(RADIO_IRQ_PIN_bit)
#define RADIO_IRQ_SET_INPUT    RADIO_IRQ_ddr &= ~RADIO_IRQ_PIN_MASK
#define RADIO_IRQ_SET_OUTPUT   RADIO_IRQ_ddr |=  RADIO_IRQ_PIN_MASK
#define RADIO_IRQ_SET_PULLUP   RADIO_IRQ_port |= RADIO_IRQ_PIN_MASK
#define IS_RADIO_IRQ_on       ((RADIO_IRQ_ipr & RADIO_IRQ_PIN_MASK) == 0x00)

#endif


