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
 
#ifndef __have__RC_RX_TX_RX_h__
#define __have__RC_RX_TX_RX_h__

#include "My_RF24.h"

#define RX_NUM_CHANNELS            8

#define CABELL_BIND_RADIO_ADDR  0xA4B7C123F7LL

#define CABELL_NUM_CHANNELS     16                  // The maximum number of RC channels that can be sent in one packet
#define CABELL_MIN_CHANNELS     4                   // The minimum number of channels that must be included in a packet, the number of channels cannot be reduced any further than this
#define CABELL_PAYLOAD_BYTES    24                  // 12 bits per value * 16 channels

#define CABELL_RADIO_CHANNELS         9                   // This is 1/5 of the total number of radio channels used for FHSS
#define CABELL_RADIO_MIN_CHANNEL_NUM  3                   // Channel 0 is right on the boarder of allowed frequency range, so move up to avoid bleeding over

#define CABELL_OPTION_MASK_MAX_POWER_OVERRIDE    0x40
#define CABELL_OPTION_MASK_CHANNEL_REDUCTION     0x0F
#define CABELL_OPTION_MASK_RECIEVER_OUTPUT_MODE  0x30
#define CABELL_OPTION_SHIFT_RECIEVER_OUTPUT_MODE 4
#define CABELL_RECIEVER_OUTPUT_PWM               0
#define CABELL_RECIEVER_OUTPUT_PPM               1
#define CABELL_RECIEVER_OUTPUT_SBUS              2

#define CHANNEL_MIN_VALUE         1000
#define CHANNEL_MAX_VALUE         2000
#define CHANNEL_MID_VALUE         ((CHANNEL_MIN_VALUE + CHANNEL_MAX_VALUE)/2)

#define PITCH_CHANNEL             0
#define ROLL_CHANNEL              1
#define YAW_CHANNEL               2
#define THROTTLE_CHANNEL          3
#define AUX1_CHANNEL              4  // only AUX1 needs to be specified here.  All subsequence AUX channels follow in sequence

#define RX_CONNECTION_TIMEOUT     1000000     // If no packet recieved in this time frame apply failsafe settings. In microseconds
#define RX_DISARM_TIMEOUT         3000000     // If no packet recieved in this time frame disarm the throttle. In microseconds

// FHSS parameters
#define DEFAULT_PACKET_INTERVAL         ((uint32_t)3000) 
#define MAX_PACKET_INTERVAL             ((uint32_t)4000) // Max packet interval - used with telemetry and 16 channels
#define INITIAL_PACKET_TIMEOUT_ADD      200ul
#define RESYNC_TIME_OUT                 ((uint32_t)2000000)                                                      //  Go to resync if no packet recieved in 2 seconds
#define RESYNC_WAIT_MICROS              (((((uint32_t)CABELL_RADIO_CHANNELS)*5ul)+8ul) * MAX_PACKET_INTERVAL)   // when syncing listen on each channel for slightly longer than the time it takes to cycle through all channels

#define INITIAL_TELEMETRY_PACKETS_TO_SKIP  1000   // dont send initial telemetry packets to avoid anoying warnings at startup

#define DO_NOT_SOFT_REBIND   0xAA

#define SERVO_OUTPUT_PINS         {PITCH_PIN,ROLL_PIN,YAW_PIN,THROTTLE_PIN,AUX1_PIN,AUX2_PIN,AUX3_PIN,AUX4_PIN}

typedef struct {
   enum RxMode_t : uint8_t {   // Note bit 8 is used to indicate if the packet is the first of 2 on the channel.  Mask out this bit before using the enum
         normal                 = 0,
         bind                   = 1,
         setFailSafe            = 2,
         normalWithTelemetry    = 3,   
         telemetryResponse      = 4,
         unBind                 = 127
   } RxMode;
   uint8_t  reserved = 0;
   uint8_t  option;
                          /*   mask 0x0F    : Channel reduction.  The number of channels to not send (subtracted frim the 16 max channels) at least 4 are always sent
                           *   mask 0x30>>4 : Reciever outout mode
                           *                  0 (00) = Single PPM on individual pins for each channel 
                           *                  1 (01) = SUM PPM on channel 1 pin
                           *                  2 (10) = Future use.  Reserved for SBUS output
                           *                  3 (11) = Unused
                           *   mask 0x40>>6   Contains max power override flag for Multiprotocol TX module. Also sent to RX
                           *   mask 0x80>>7   Unused 
                           */  
   uint8_t  modelNum;
   uint8_t  checkSum_LSB;   // Checksum least significant byte
   uint8_t  checkSum_MSB;   // Checksum most significant byte
   uint8_t  payloadValue [CABELL_PAYLOAD_BYTES] = {0}; //12 bits per channel value, unsigned
} CABELL_RxTxPacket_t;   

void setupReciever();
void outputSumPPM();
void outputPWM();
void outputSbus();
bool getPacket();
void outputFailSafeValues(bool callOutputChannels);
void outputChannels();
void attachServoPins();
void detachServoPins();
void setNextRadioChannel(bool missedPacket);
void checkFailsafeDisarmTimeout(unsigned long lastPacketTime,bool inititalGoodPacketRecieved);
void unbindReciever();
void bindReciever(uint8_t modelNum, uint16_t tempHoldValues[]);
bool validateChecksum(CABELL_RxTxPacket_t const& packet, uint8_t maxPayloadValueIndex);
bool readAndProcessPacket();
bool decodeChannelValues(CABELL_RxTxPacket_t const& RxPacket, uint8_t channelsRecieved, uint16_t tempHoldValues[]);
bool processRxMode (uint8_t RxMode, uint8_t modelNum, uint16_t tempHoldValues[]);
void setFailSafeDefaultValues();
void loadFailSafeDefaultValues();
void setFailSafeValues(uint16_t newFailsafeValues[]);
void setNewDataRate();
unsigned long  sendTelemetryPacket();    //returns micros of when the transmit is esspected to be complete
uint8_t calculateRSSI(bool goodPacket);
void ADC_Processing();
bool failSafeButtonHeld();
void setTelemetryPowerMode(uint8_t option);
void initializeRadio(My_RF24* radio);
void swapRecievers();

#endif

