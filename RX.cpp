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
 
#include <SPI.h>
#include "My_RF24.h" 
#include "RX.h"
#include "Pins.h"
#include <EEPROM.h>
#include "Rx_Tx_Util.h"
#include "SUM_PPM.h"
#include "MyServo.h"    // hacked to remove timer1 ISR - must call this in my own ISR
#include "TestHarness.h" 
#include "RSSI.h" 
#include "My_nRF24L01.h"
#include "SBUS.h"

My_RF24 radio1(RADIO1_CSN_PIN,RADIO1_CSN_PIN);  
My_RF24 radio2(RADIO2_CSN_PIN,RADIO2_CSN_PIN);  

My_RF24* primaryReciever = NULL;
My_RF24* secondaryReciever = NULL;

uint8_t radioConfigRegisterForTX = 0;
uint8_t radioConfigRegisterForRX_IRQ_Masked = 0;
uint8_t radioConfigRegisterForRX_IRQ_On = 0;
  
uint16_t channelValues [CABELL_NUM_CHANNELS];
uint8_t  currentModel = 0;
uint64_t radioPipeID;
uint64_t radioNormalRxPipeID;

const int currentModelEEPROMAddress = 0;
const int radioPipeEEPROMAddress = currentModelEEPROMAddress + sizeof(currentModel);
const int softRebindFlagEEPROMAddress = radioPipeEEPROMAddress + sizeof(radioNormalRxPipeID);
const int failSafeChannelValuesEEPROMAddress = softRebindFlagEEPROMAddress + sizeof(uint8_t);       // uint8_t is the sifr of the rebind flag

uint16_t failSafeChannelValues [CABELL_NUM_CHANNELS];

bool throttleArmed = true;
bool bindMode = false;     // when true send bind command to cause reciever to bind enter bind mode
bool failSafeDisplayFlag = true;
bool packetMissed = false;
bool fastDataRate = true;
uint32_t packetInterval = DEFAULT_PACKET_INTERVAL;

uint8_t radioChannel[CABELL_RADIO_CHANNELS];

volatile uint8_t currentOutputMode = 255;    // initialize to an unused mode
volatile uint8_t nextOutputMode = 255;       // initialize to an unused mode

volatile bool packetReady = false;

bool telemetryEnabled = false;
int16_t analogValue[2] = {0,0};

uint16_t initialTelemetrySkipPackets = 0;

MyServo channelServo[RX_NUM_CHANNELS];

#ifdef TEST_HARNESS
  TestHarness testOut;
#endif

RSSI rssi;

//--------------------------------------------------------------------------------------------------------------------------
void attachServoPins() {

  uint8_t servoPin[RX_NUM_CHANNELS] = SERVO_OUTPUT_PINS;

  for (uint8_t x = 0; x < RX_NUM_CHANNELS; x++)
    channelServo[x].attach(servoPin[x]);

}

//--------------------------------------------------------------------------------------------------------------------------
void detachServoPins() {

  for (uint8_t x = 0; x < RX_NUM_CHANNELS; x++)
    channelServo[x].detach();
}

//--------------------------------------------------------------------------------------------------------------------------
void setupReciever() {  
  uint8_t softRebindFlag;

  EEPROM.get(softRebindFlagEEPROMAddress,softRebindFlag);
  EEPROM.get(radioPipeEEPROMAddress,radioNormalRxPipeID);
  EEPROM.get(currentModelEEPROMAddress,currentModel);
 
  if ((digitalRead(BIND_BUTTON_PIN) == LOW) || (softRebindFlag != DO_NOT_SOFT_REBIND)) {
    bindMode = true;
    radioPipeID = CABELL_BIND_RADIO_ADDR;
    Serial.println (F("Bind Mode "));
    digitalWrite(LED_PIN, HIGH);      // Turn on LED to indicate bind mode
    radioNormalRxPipeID = 0x01<<43;   // This is a number bigger than the max possible pipe ID, which only uses 40 bits.  This makes sure the bind routine writes to EEPROM
  }
  else
  {
    bindMode = false;
    radioPipeID = radioNormalRxPipeID;    
  }

  getChannelSequence (radioChannel, CABELL_RADIO_CHANNELS, radioPipeID);

  //Need to set all CSN pins high before BEGIN so that only one device listens on SPI during the first initialization
  pinMode(RADIO1_CSN_PIN,OUTPUT);    
  pinMode(RADIO2_CSN_PIN,OUTPUT);   
  digitalWrite(RADIO1_CSN_PIN,HIGH);
  digitalWrite(RADIO2_CSN_PIN,HIGH);
  
  radio1.begin();
  radio2.begin();

  // Set primary and secondary recievers.
  // If only one is present, then both primary and secondary reciever pointers end up pointing to the same radio.
  // This way the reciecer swap logic doesn't care if one or two recieves are actually connected
  if (radio2.isChipConnected()) {
    Serial.println(F("Backup radio detected"));
    secondaryReciever = &radio2;
  } else {
    Serial.println(F("Backup radio not detected"));
    secondaryReciever = &radio1;
    digitalWrite(RADIO2_CSN_PIN,HIGH);   // If the backup radio is not present, set this pin high becasue some older 1 radio configurations used this as CE on the primary radio
  }  
  if (radio1.isChipConnected()) {
    primaryReciever = &radio1;
    Serial.println(F("Primary radio detected"));
  } else {
    primaryReciever = &radio2;
    Serial.println(F("Primary radio not detected"));
  }

  RADIO_IRQ_SET_INPUT;
  RADIO_IRQ_SET_PULLUP;

  initializeRadio(primaryReciever);
  initializeRadio(secondaryReciever);

  setNewDataRate();
  setTelemetryPowerMode(CABELL_OPTION_MASK_MAX_POWER_OVERRIDE);
  
  primaryReciever->flush_rx();
  secondaryReciever->flush_rx();
  packetReady = false;

  outputFailSafeValues(false);   // initialize default values for output channels
  
  Serial.print(F("Radio ID: "));Serial.print((uint32_t)(radioPipeID>>32)); Serial.print(F("    "));Serial.println((uint32_t)((radioPipeID<<32)>>32));
  Serial.print(F("Current Model Number: "));Serial.println(currentModel);
  
 //setup pin change interrupt
  cli();    // switch interrupts off while messing with their settings  
  PCICR =0x02;          // Enable PCINT1 interrupt
  PCMSK1 = RADIO_IRQ_PIN_MASK;
  sei();

  #ifdef TEST_HARNESS
    testOut.init();
    Serial.println(F("Operating as a test harness.  Servo output disabled."));
  #endif

}

//--------------------------------------------------------------------------------------------------------------------------
  ISR(PCINT1_vect) {
    if (IS_RADIO_IRQ_on)  {  // pulled low when packet is recieved
      packetReady = true;
    }
  }

//--------------------------------------------------------------------------------------------------------------------------
void outputChannels() {
#ifndef TEST_HARNESS
    if (!throttleArmed) {
      channelValues[THROTTLE_CHANNEL] = CHANNEL_MIN_VALUE;     // Safety precaution.  Min throttle if not armed
    }
    
    bool firstPacketOnMode = false;
  
    if (currentOutputMode != nextOutputMode) {   // If new mode, turn off all modes
     firstPacketOnMode = true;
     detachServoPins();
     if (PPMEnabled()) ppmDisable();
    }
  
    if (nextOutputMode == 0) {
      outputPWM();                               // Do this first so we have something to send when PWM enabled
      if (firstPacketOnMode) {                   // First time through attach pins to start output
        attachServoPins();
      }
    }
  
    if (nextOutputMode == 1) {
      outputSumPPM();                           // Do this first so we have something to send when PPM enabled
      if (firstPacketOnMode) {
        if (!PPMEnabled()) {
          ppmSetup(PPM_OUTPUT_PIN, RX_NUM_CHANNELS);
        }
      }
    }
  
    if (nextOutputMode == 2) {
      outputSbus();                           // Do this first so we have something to send when SBUS enabled
      if (firstPacketOnMode) {
        if (!sbusEnabled()) {
          sbusSetup();
        }
      }
    }
    currentOutputMode = nextOutputMode;
#endif
}

//--------------------------------------------------------------------------------------------------------------------------
void setNextRadioChannel(bool missedPacket) {
  static int currentChannel = CABELL_RADIO_MIN_CHANNEL_NUM;  // Initializes the channel sequence.
  
  //primaryReciever->stopListening();
  primaryReciever->write_register(NRF_CONFIG,radioConfigRegisterForTX);  // This is in place of stop listening to make the change to TX more quickly. Also sets all interrupts to mask
  primaryReciever->flush_rx();
  unsigned long expectedTransmitCompleteTime = 0;
  if (telemetryEnabled) {
    if (initialTelemetrySkipPackets >= INITIAL_TELEMETRY_PACKETS_TO_SKIP) {  // dont send the first 500 telemetry packets to avoid anoying warnigns at startup
      expectedTransmitCompleteTime = sendTelemetryPacket();
    } else {
      initialTelemetrySkipPackets++;
    }
  }

  //only swap recievers if the secondary reciever got the last packet 
  //so we dont swap to a reciever that is not currently revieving
  //unless packet was missed by both radios, in which case swap every time.

  bool performSwap = secondaryReciever->available() || missedPacket;
  
  currentChannel = getNextChannel (radioChannel, CABELL_RADIO_CHANNELS, currentChannel);
  
  if (telemetryEnabled && expectedTransmitCompleteTime != 0) {
   // Wait here for the telemetry packet to finsish transmitting
   long waitTimeLeft = (long)(expectedTransmitCompleteTime - micros());
   if (waitTimeLeft > 0) {
    delayMicroseconds(waitTimeLeft);
    }
  }
  
  //secondaryReciever->stopListening();
  secondaryReciever->write_register(NRF_CONFIG,radioConfigRegisterForTX);  // This is in place of stop listening to make the change to TX more quickly. Also sets all interrupts to mask.
  secondaryReciever->flush_rx();
  secondaryReciever->setChannel(currentChannel);
  //secondaryReciever->startListening();
  secondaryReciever->write_register(NRF_CONFIG,radioConfigRegisterForRX_IRQ_Masked);  // This is in place of stop listening to make the change to TX more quickly. Also sets all interrupts to mask.
  secondaryReciever->write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );                 // This normally happens in StartListening

  primaryReciever->setChannel(currentChannel);
  //primaryReciever->startListening();
  primaryReciever->write_register(NRF_CONFIG,radioConfigRegisterForRX_IRQ_Masked);  // This is in place of stop listening to make the change to TX more quickly. Also sets all interrupts to mask.
  primaryReciever->write_register(NRF_STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );                 // This normally happens in StartListening
  if (performSwap) {
    swapRecievers();
   }

 primaryReciever->write_register(NRF_CONFIG,radioConfigRegisterForRX_IRQ_On);  // Turn on RX interrupt
}

//--------------------------------------------------------------------------------------------------------------------------
bool getPacket() {  
  static unsigned long lastPacketTime = 0;  
  static bool inititalGoodPacketRecieved = false;
  static unsigned long nextAutomaticChannelSwitch = micros() + RESYNC_WAIT_MICROS;
  static unsigned long lastRadioPacketeRecievedTime = 0;
  bool goodPacket_rx = false;
  
  // Wait for the radio to get a packet, or the timeout for the current radio channel occurs
  if (!packetReady) {
    if ((long)(micros() - nextAutomaticChannelSwitch) >= 0 ) {      // if timed out the packet was missed, go to the next channel
      if (secondaryReciever->available()) {
        // missed packet but secondary radio has it so swap radios and signal packet ready
        //packet will be picked up on next loop through
        packetReady = true;
        swapRecievers();
        //Serial.println("SR);
        rssi.secondaryHit();
        #ifdef TEST_HARNESS
           testOut.secondaryHit();
        #endif
      } else {
        //if (inititalGoodPacketRecieved) Serial.println("miss");
        packetMissed = true;
        rssi.miss();
        #ifdef TEST_HARNESS
           testOut.miss();
        #endif
        setNextRadioChannel(true);       //true indicated that packet was missed         
        if ((long)(nextAutomaticChannelSwitch - lastRadioPacketeRecievedTime) > ((long)RESYNC_TIME_OUT)) {  // if a long time passed, increase timeout duration to re-sync with the TX
          telemetryEnabled = false;
          #ifdef TEST_HARNESS
            testOut.reSync();
          #else
            Serial.println(F("Re-sync Attempt"));
          #endif
          packetInterval = DEFAULT_PACKET_INTERVAL;
          initialTelemetrySkipPackets = 0;
          nextAutomaticChannelSwitch += RESYNC_WAIT_MICROS;
          setNewDataRate();                       //Alternate data rates until signal found
        } else {
          nextAutomaticChannelSwitch += packetInterval;
        }
        checkFailsafeDisarmTimeout(lastPacketTime,inititalGoodPacketRecieved);   // at each timeout, check for failsafe and disarm.  When disarmed TX must send min throttle to re-arm.
      }
    }
  } else {
    lastRadioPacketeRecievedTime = micros();   //Use this time to calculate the next expected packet so when we miss packets we can change channels
    //if (inititalGoodPacketRecieved) Serial.println("hit");
    goodPacket_rx = readAndProcessPacket();
    nextAutomaticChannelSwitch = lastRadioPacketeRecievedTime + packetInterval + INITIAL_PACKET_TIMEOUT_ADD; 
    packetReady = false;
    if (goodPacket_rx) {
      inititalGoodPacketRecieved = true;
      lastPacketTime = micros();
      failSafeDisplayFlag = true;
      packetMissed = false;
      rssi.hit();
      #ifdef TEST_HARNESS
        testOut.hit();
      #endif
    } else {
      rssi.badPacket();
      packetMissed = true;
      #ifdef TEST_HARNESS
        testOut.badPacket();
      #endif
    }
  }
  return goodPacket_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
void checkFailsafeDisarmTimeout(unsigned long lastPacketTime,bool inititalGoodPacketRecieved) {
  unsigned long holdMicros = micros();

  if ((long)(holdMicros - lastPacketTime)  > ((long)RX_CONNECTION_TIMEOUT)) {  
    outputFailSafeValues(true);
  }
  
  if (((long)(holdMicros - lastPacketTime) >  ((long)RX_DISARM_TIMEOUT)) || (!inititalGoodPacketRecieved && ((long)(holdMicros - lastPacketTime)  > ((long)RX_DISARM_TIMEOUT)) ) ) { 
    if (throttleArmed) {
      Serial.println(F("Disarming throttle"));
      throttleArmed = false;
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------------
void outputPWM() {

  channelServo[ROLL_CHANNEL].writeMicroseconds(channelValues[ROLL_CHANNEL]);
  channelServo[PITCH_CHANNEL].writeMicroseconds(channelValues[PITCH_CHANNEL]);
  channelServo[YAW_CHANNEL].writeMicroseconds(channelValues[YAW_CHANNEL]);
  channelServo[THROTTLE_CHANNEL].writeMicroseconds(channelValues[THROTTLE_CHANNEL]);

  for (uint8_t x = AUX1_CHANNEL; x < RX_NUM_CHANNELS; x++) 
    channelServo[x].writeMicroseconds(channelValues[x]);

}

//--------------------------------------------------------------------------------------------------------------------------
void outputSumPPM() {  // output as AETR
  int adjusted_x;
  for(int x = 0; x < min(RX_NUM_CHANNELS,CABELL_NUM_CHANNELS) ; x++) {  // 

    //set adjusted_x to be in AETR order
    switch (x)
    {
      case 0:
        adjusted_x = ROLL_CHANNEL;
        break;
      case 1:
        adjusted_x = PITCH_CHANNEL;
        break;
      case 2:
        adjusted_x = THROTTLE_CHANNEL;
        break;
      case 3:
        adjusted_x = YAW_CHANNEL;
        break;
      default:
        adjusted_x = x;
    }
    
    setPPMOutputChannelValue(x, channelValues[adjusted_x]);
  //  Serial.print(channelValues[x]); Serial.print("\t"); 
  } 
}

//--------------------------------------------------------------------------------------------------------------------------
void outputSbus() {  // output as AETR
  int adjusted_x;
  for(int x = 0; x < CABELL_NUM_CHANNELS ; x++) {  // 

    //set adjusted_x to be in AETR order
    switch (x)
    {
      case 0:
        adjusted_x = ROLL_CHANNEL;
        break;
      case 1:
        adjusted_x = PITCH_CHANNEL;
        break;
      case 2:
        adjusted_x = THROTTLE_CHANNEL;
        break;
      case 3:
        adjusted_x = YAW_CHANNEL;
        break;
      default:
        adjusted_x = x;
    }
    
    setSbusOutputChannelValue(x, channelValues[adjusted_x]);
  //  Serial.print(channelValues[x]); Serial.print("\t"); 
  }
  sbusSetFailsafe(!failSafeDisplayFlag); 
  sbusSetFrameLost(packetMissed); 
}

//--------------------------------------------------------------------------------------------------------------------------
void outputFailSafeValues(bool callOutputChannels) {

  loadFailSafeDefaultValues();

  //Serial.println("FS Vals");
  for (uint8_t x =0; x < CABELL_NUM_CHANNELS; x++) {
    channelValues[x] = failSafeChannelValues[x];
    //Serial.println(channelValues[x]);
  }

  if (failSafeDisplayFlag) {  
    #ifdef TEST_HARNESS
      testOut.failSafe();
    #else
      Serial.println(F("Failsafe"));
    #endif
    failSafeDisplayFlag = false;
  }
  if (callOutputChannels)
    outputChannels();
}

//--------------------------------------------------------------------------------------------------------------------------
#ifndef TEST_HARNESS
  ISR(TIMER1_COMPA_vect){
    if (currentOutputMode == 0)
      MyServoInterruptOneProcessing();
    else if (currentOutputMode == 1)
      SUM_PPM_ISR();
    else if (currentOutputMode == 2)
      SBUS_ISR();
  }
#endif

//--------------------------------------------------------------------------------------------------------------------------
void unbindReciever() {
  // Reset all of flash memory to unbind reciever
  uint8_t value = 0xFF;
  Serial.print(F("Overwriting flash with value "));Serial.println(value, HEX);
  for (int x = 0; x < 1024; x++) {
        EEPROM.put(x,value);
  }
  
  Serial.println(F("Reciever un-bound.  Reboot to enter bind mode"));
  outputFailSafeValues(true);
  bool ledState = false;
  while (true) {                                           // Flash LED forever indicating unbound
    digitalWrite(LED_PIN, ledState);
    ledState =  !ledState;
    delay(250);                                            // Fast LED flash
  }  
}

//--------------------------------------------------------------------------------------------------------------------------
void bindReciever(uint8_t modelNum, uint16_t tempHoldValues[]) {
  // new radio address is in channels 11 to 15
  uint64_t newRadioPipeID = (((uint64_t)(tempHoldValues[11]-1000)) << 32) + 
                            (((uint64_t)(tempHoldValues[12]-1000)) << 24) + 
                            (((uint64_t)(tempHoldValues[13]-1000)) << 16) + 
                            (((uint64_t)(tempHoldValues[14]-1000)) << 8)  + 
                            (((uint64_t)(tempHoldValues[15]-1000)));    // Address to use after binding
                
  if ((modelNum != currentModel) || (radioNormalRxPipeID != newRadioPipeID)) {
    EEPROM.put(currentModelEEPROMAddress,modelNum);
    radioNormalRxPipeID = newRadioPipeID;
    EEPROM.put(radioPipeEEPROMAddress,radioNormalRxPipeID);
    Serial.print(F("Bound to new TX address for model number "));Serial.println(modelNum);
    digitalWrite(LED_PIN, LOW);                              // Turn off LED to indicate sucessful bind
    EEPROM.put(softRebindFlagEEPROMAddress,(uint8_t)DO_NOT_SOFT_REBIND);
    setFailSafeDefaultValues();
    outputFailSafeValues(true);
    Serial.println(F("Reciever bound.  Reboot to enter normal mode"));
    bool ledState = false;
    while (true) {                                           // Flash LED forever indicating bound
      digitalWrite(LED_PIN, ledState);
      ledState =  !ledState;
      delay(2000);                                            // Slow flash
    }
  }  
}

//--------------------------------------------------------------------------------------------------------------------------
void setFailSafeDefaultValues() {
  uint16_t defaultFailSafeValues[CABELL_NUM_CHANNELS];
  for (int x = 0; x < CABELL_NUM_CHANNELS; x++) {
    defaultFailSafeValues[x] = CHANNEL_MID_VALUE;
  }
  defaultFailSafeValues[THROTTLE_CHANNEL] = CHANNEL_MIN_VALUE;           // Throttle should always be the min value when failsafe}
  setFailSafeValues(defaultFailSafeValues);  
}

//--------------------------------------------------------------------------------------------------------------------------
void loadFailSafeDefaultValues() {
  EEPROM.get(failSafeChannelValuesEEPROMAddress,failSafeChannelValues);
  for (int x = 0; x < CABELL_NUM_CHANNELS; x++) {
    if (failSafeChannelValues[x] < CHANNEL_MIN_VALUE || failSafeChannelValues[x] > CHANNEL_MAX_VALUE) {    // Make sure failsafe values are valid
      failSafeChannelValues[x] = CHANNEL_MID_VALUE;
    }
  }
  failSafeChannelValues[THROTTLE_CHANNEL] = CHANNEL_MIN_VALUE;     // Throttle should always be the min value when failsafe
}

//--------------------------------------------------------------------------------------------------------------------------
void setFailSafeValues(uint16_t newFailsafeValues[]) {
    for (int x = 0; x < CABELL_NUM_CHANNELS; x++) {
      failSafeChannelValues[x] = newFailsafeValues[x];
    }
    failSafeChannelValues[THROTTLE_CHANNEL] = CHANNEL_MIN_VALUE;           // Throttle should always be the min value when failsafe}
    EEPROM.put(failSafeChannelValuesEEPROMAddress,failSafeChannelValues);  
    Serial.println(F("Fail Safe Values Set"));
}

//--------------------------------------------------------------------------------------------------------------------------
bool validateChecksum(CABELL_RxTxPacket_t const& packet, uint8_t maxPayloadValueIndex) {
  //caculate checksum and validate
  uint16_t packetSum = packet.modelNum + packet.option + packet.RxMode + packet.reserved;    
    
  for (int x = 0; x < maxPayloadValueIndex; x++) {
    packetSum = packetSum +  packet.payloadValue[x];
  }

  if (packetSum != ((((uint16_t)packet.checkSum_MSB) <<8) + (uint16_t)packet.checkSum_LSB)) {  
    //Serial.print("Checksum Error "); Serial.print(packetSum); Serial.print(" expected ");  Serial.println(packet.checkSum); 
    return false;       // dont take packet if checksum bad  
  } 
  else
  {
    return true;
  }
}

//--------------------------------------------------------------------------------------------------------------------------
bool readAndProcessPacket() {    //only call when a packet is available on the radio
  CABELL_RxTxPacket_t RxPacket;
  
  primaryReciever->read( &RxPacket,  sizeof(RxPacket) );
  setNextRadioChannel(false);                   // Also sends telemetry if in telemetry mode.  Doing this as soon as possible to keep timing as tight as possible
                                                // False indicates that packet was not missed

  // Remove 8th bit from RxMode because this is a toggle bit that is not included in the checksum
  // This toggle with each xmit so consecrutive payloads are not identical.  This is a work around for a reported bug in clone NRF24L01 chips that mis-took this case for a re-transmit of the same packet.
  uint8_t* p = reinterpret_cast<uint8_t*>(&RxPacket.RxMode);
  *p &= 0x7F;  //ensure 8th bit is not set.  This bit is not included in checksum

  // putting this after setNextRadioChannel will lag by one telemetry packet, but by doing this the telemetry can be sent sooner, improving the timing
  telemetryEnabled = (RxPacket.RxMode==CABELL_RxTxPacket_t::RxMode_t::normalWithTelemetry)?true:false;
                                                
  bool packet_rx = false;
  uint16_t tempHoldValues [CABELL_NUM_CHANNELS]; 
  uint8_t channelReduction = constrain((RxPacket.option & CABELL_OPTION_MASK_CHANNEL_REDUCTION),0,CABELL_NUM_CHANNELS-CABELL_MIN_CHANNELS);  // Must be at least 4 channels, so cap at 12
  uint8_t packetSize = sizeof(RxPacket) - ((((channelReduction - (channelReduction%2))/ 2)) * 3);      // reduce 3 bytes per 2 channels, but not last channel if it is odd
  uint8_t maxPayloadValueIndex = sizeof(RxPacket.payloadValue) - (sizeof(RxPacket) - packetSize);
  uint8_t channelsRecieved = CABELL_NUM_CHANNELS - channelReduction; 
  
  if (telemetryEnabled) {  // putting this after setNextRadioChannel will lag by one telemetry packet, but by doing this the telemetry can be sent sooner, improving the timing
    setTelemetryPowerMode(RxPacket.option);
    packetInterval = DEFAULT_PACKET_INTERVAL + (constrain(((int16_t)channelsRecieved - (int16_t)6),(int16_t)0 ,(int16_t)10 ) * (int16_t)100);  // increase packet period by 100 us for each channel over 6
  } else {
    packetInterval = DEFAULT_PACKET_INTERVAL;
  }

  packet_rx = validateChecksum(RxPacket, maxPayloadValueIndex);

  if (packet_rx)
    packet_rx = decodeChannelValues(RxPacket, channelsRecieved, tempHoldValues);

  if (packet_rx) 
    packet_rx = processRxMode (RxPacket.RxMode, RxPacket.modelNum, tempHoldValues);    // If bind or unbind happens, this will never return.

  // if packet is good, copy the channel values
  if (packet_rx) {
    nextOutputMode = (RxPacket.option & CABELL_OPTION_MASK_RECIEVER_OUTPUT_MODE) >> CABELL_OPTION_SHIFT_RECIEVER_OUTPUT_MODE;
    for ( int b = 0 ; b < CABELL_NUM_CHANNELS ; b ++ ) { 
      channelValues[b] =  (b < channelsRecieved) ? tempHoldValues[b] : CHANNEL_MID_VALUE;   // use the mid value for channels not recieved.
    }
  } 
  else
  {
    Serial.println("RX Pckt Err");    // Dont use F macro here.  Want this to be fast as it is in the main loop logic
  }  

  return packet_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
bool processRxMode (uint8_t RxMode, uint8_t modelNum, uint16_t tempHoldValues[]) {
  static bool failSafeValuesHaveBeenSet = false;
  bool packet_rx = true;

  // fail safe settings can come in on a failsafe packet, but also use a normal packed if bind mode button is pressed after start up
  if (failSafeButtonHeld()) { 
    if (RxMode == CABELL_RxTxPacket_t::RxMode_t::normal || RxMode == CABELL_RxTxPacket_t::RxMode_t::normalWithTelemetry) {
      RxMode = CABELL_RxTxPacket_t::RxMode_t::setFailSafe;
    }
  }

  switch (RxMode) {
    case CABELL_RxTxPacket_t::RxMode_t::bind   :  if (bindMode) {
                                                    bindReciever(modelNum, tempHoldValues);
                                                  }
                                                  else
                                                  {
                                                    Serial.println(F("Bind command detected but reciever not in bind mode"));
                                                    packet_rx = false;
                                                  }
                                                  break;
                                              
    case CABELL_RxTxPacket_t::RxMode_t::setFailSafe :  if (modelNum == currentModel) {                                                         
                                                         digitalWrite(LED_PIN, HIGH);
                                                         if (!failSafeValuesHaveBeenSet) {      // only set the values first time through
                                                           failSafeValuesHaveBeenSet = true;
                                                           setFailSafeValues(tempHoldValues);
                                                         }
                                                       } 
                                                       else 
                                                       {
                                                         packet_rx = false;
                                                         Serial.println(F("Wrong Model Number"));
                                                       }
                                                       break;
                                                      
    case CABELL_RxTxPacket_t::RxMode_t::normalWithTelemetry : 
    case CABELL_RxTxPacket_t::RxMode_t::normal :  if (modelNum == currentModel) {
                                                    digitalWrite(LED_PIN, LOW);
                                                    failSafeValuesHaveBeenSet = false;             // Reset when not in setFailSafe mode so next time failsafe is to be set it will take
                                                    if (!throttleArmed && (tempHoldValues[THROTTLE_CHANNEL] <= CHANNEL_MIN_VALUE + 10)) { 
                                                      Serial.println("Throttle Armed");             // Dont use F macro here.  Want this to be fast as it is in the main loop logic
                                                      throttleArmed = true;
                                                    }
                                                  } 
                                                  else 
                                                  {
                                                    packet_rx = false;
                                                    Serial.println(F("Wrong Model Number"));
                                                  }
                                                  break;
                                                  
    case CABELL_RxTxPacket_t::RxMode_t::unBind :  if (modelNum == currentModel) {
                                                    unbindReciever();
                                                  } else {
                                                    packet_rx = false;
                                                    Serial.println(F("Wrong Model Number"));
                                                 }
                                                  break;
    default : Serial.println(F("Unknown RxMode"));
              packet_rx = false;
              break;          
  }
  return packet_rx;  
}

//--------------------------------------------------------------------------------------------------------------------------
bool decodeChannelValues(CABELL_RxTxPacket_t const& RxPacket, uint8_t channelsRecieved, uint16_t tempHoldValues[]) {
  // decode the 12 bit numbers to temp array. 
  bool packet_rx = true;
  int payloadIndex = 0;
  
  for ( int b = 0 ; (b < channelsRecieved); b ++ ) { 
    tempHoldValues[b] = RxPacket.payloadValue[payloadIndex];  
    payloadIndex++;
    tempHoldValues[b] |= ((uint16_t)RxPacket.payloadValue[payloadIndex]) <<8;  
    if (b % 2) {     //channel number is ODD
       tempHoldValues[b] = tempHoldValues[b]>>4;
       payloadIndex++;
    } else {         //channel number is EVEN
       tempHoldValues[b] &= 0x0FFF;
    }
    if ((tempHoldValues[b] > CHANNEL_MAX_VALUE) || (tempHoldValues[b] < CHANNEL_MIN_VALUE)) { 
      packet_rx = false;   // throw out entire packet if any value out of range
      //Serial.print("Value Error Channel: ");Serial.print(b);Serial.print(" Value: ");Serial.println(tempHoldValues[b]); 
    }
  }  
  
//  for ( int b = 0 ; b < CABELL_NUM_CHANNELS ; b ++ ) { 
//    Serial.print(tempHoldValues[b]);Serial.print("\t");
//  }
  
  return packet_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
void setNewDataRate() {

  fastDataRate = !fastDataRate;

  if (fastDataRate) {
    primaryReciever->setDataRate(RF24_1MBPS);
    secondaryReciever->setDataRate(RF24_1MBPS);
  } else {
    primaryReciever->setDataRate(RF24_250KBPS );
    secondaryReciever->setDataRate(RF24_250KBPS );
  }
  //setDataRate changes txDelay so reset it here  
  primaryReciever->txDelay = 0;         // Timing works out so a delay is not needed
  secondaryReciever->txDelay = 0;       

}

//--------------------------------------------------------------------------------------------------------------------------
unsigned long sendTelemetryPacket() {

  static int8_t packetCounter = 0;  // this is only used for toggleing bit
  uint8_t sendPacket[4] = {CABELL_RxTxPacket_t::RxMode_t::telemetryResponse};
 
  packetCounter++;
  sendPacket[0] &= 0x7F;                 // clear 8th bit
  sendPacket[0] |= packetCounter<<7;     // This causes the 8th bit of the first byte to toggle with each xmit so consecutive payloads are not identical.  This is a work around for a reported bug in clone NRF24L01 chips that mis-took this case for a re-transmit of the same packet.
  sendPacket[1] = rssi.getRSSI();   
  sendPacket[2] = analogValue[0]/4;      // Send a 8 bit value (0 to 255) of the analog input.  Can be used for Lipo voltage or other analog input for telemetry
  sendPacket[3] = analogValue[1]/4;      // Send a 8 bit value (0 to 255) of the analog input.  Can be used for Lipo voltage or other analog input for telemetry

  //Serial.print(analogValue[0]);
  //Serial.print(" ");
  //Serial.println(analogValue[1]);

  uint8_t packetSize =  sizeof(sendPacket);
  primaryReciever->startFastWrite( &sendPacket[0], packetSize, 0); 

      // calculate transmit time based on packet size and data rate of 1MB per sec
      // This is done becasue polling the status register during xmit to see when xmit is done casued issues sometimes.
      // bits = packstsize * 8  +  73 bits overhead
      // at 1 MB per sec, one bit is 1 uS
      // then add 140 uS which is 130 uS to begin the xmit and 10 uS fudge factor
      // Add this to micros() to return when the transmit is esspected to be complete
      if (fastDataRate) {
        return micros() + (((unsigned long)packetSize * 8ul)  +  73ul + 140ul)   ;
      } else {
        return micros() + (((((unsigned long)packetSize * 8ul)  +  73ul) * 4ul) + 140ul)   ;  //250Kbps takes 4 times longer to xmit
      }
}


//--------------------------------------------------------------------------------------------------------------------------
// based on ADC Interrupt example from https://www.gammon.com.au/adc
void ADC_Processing() {             //Reads ADC value then configures next converion. Alternates between pins A6 and A7
  static byte adcPin = TELEMETRY_ANALOG_INPUT_1;

  if (bit_is_clear(ADCSRA, ADSC)) {
    analogValue[(adcPin==TELEMETRY_ANALOG_INPUT_1) ? 0 : 1] = ADC;  
  
    adcPin = (adcPin==TELEMETRY_ANALOG_INPUT_2) ? TELEMETRY_ANALOG_INPUT_1 : TELEMETRY_ANALOG_INPUT_2;   // Choose next pin to read
  
    ADCSRA =  bit (ADEN);                      // turn ADC on
    ADCSRA |= bit (ADPS0) |  bit (ADPS1) | bit (ADPS2);  // Prescaler of 128
    ADMUX  =  bit (REFS0) | (adcPin & 0x07);    // AVcc and select input port
  
    ADCSRA |= bit (ADSC);   //Start next conversion 
  }
}

//--------------------------------------------------------------------------------------------------------------------------
bool failSafeButtonHeld() {
  // use the bind button becasue bind mode is only checked at startup.  Once RX is started and not in bind mode it is the set failsafe button
  
  static unsigned long heldTriggerTime = 0;
  
  if(!bindMode && !digitalRead(BIND_BUTTON_PIN)) {  // invert becasue pin is pulled up so low means pressed
    if (heldTriggerTime == 0) {
      heldTriggerTime = micros() + 1000000ul;   // Held state achieved after button is pressed for 1 second
    }
    if ((long)(micros() - heldTriggerTime) >= 0) {
      return true;
    } else {
      return false;
    }
  }
  heldTriggerTime = 0;
  return false;
}

//--------------------------------------------------------------------------------------------------------------------------
void setTelemetryPowerMode(uint8_t option) {
  // Set transmit power tyo max or high based on the option byte in the incomming packet.
  // This should set the power the same as the transmitter module

  static uint8_t prevPower = RF24_PA_MIN;
  uint8_t newPower;
  if ((option & CABELL_OPTION_MASK_MAX_POWER_OVERRIDE) == 0) {
    newPower = RF24_PA_HIGH;
  } else {
    newPower = RF24_PA_MAX;
  }
  if (newPower != prevPower) {
      primaryReciever->setPALevel(newPower);
      secondaryReciever->setPALevel(newPower);
      prevPower = newPower;
  }
}

//--------------------------------------------------------------------------------------------------------------------------
void initializeRadio(My_RF24* radioPointer) {
  radioPointer->maskIRQ(true,true,true);         // Mask all interrupts.  RX interrupt (the only one we use) gets turned on after channel change
  radioPointer->enableDynamicPayloads();
  radioPointer->setChannel(0);                    // start out on a channel we dont use so we dont start recieving packets yet.  It will get changed when the looping starts
  radioPointer->setAutoAck(0);
  radioPointer->openWritingPipe(~radioPipeID);   // Invert bits for writing pipe so that telemetry packets transmit with a different pipe ID.
  radioPointer->openReadingPipe(1,radioPipeID);
  radioPointer->startListening();
  radioPointer->csDelay = 0;         //Can be reduced to 0 beacase we use interrupts and timing instead of polling through SPI

  //Stop listening to set up module for writing then take a copy of the config register so we can change to write mode more quickly when sending telemetry packets
  radioPointer->stopListening();
  radioConfigRegisterForTX = radioPointer->read_register(NRF_CONFIG);  // This saves the config register state with all interrupts masked and in TX mode.  Used to switch quickly to TX mode for telemetry
  radioPointer->startListening();
  radioConfigRegisterForRX_IRQ_Masked = radioPointer->read_register(NRF_CONFIG);  // This saves the config register state with all interrupts masked and in RX mode.  Used to switch secondary radio quickly to RX after channel change
  radioPointer->maskIRQ(true,true,false);       
  radioConfigRegisterForRX_IRQ_On = radioPointer->read_register(NRF_CONFIG);     // This saves the config register state with Read Interrupt ON and in RX mode.  Used to switch primary radio quickly to RX after channel change
  radioPointer->maskIRQ(true,true,true);       
}

//--------------------------------------------------------------------------------------------------------------------------
void swapRecievers() {
  My_RF24* hold = NULL;
  hold = primaryReciever;
  primaryReciever = secondaryReciever;
  secondaryReciever = hold;
}








