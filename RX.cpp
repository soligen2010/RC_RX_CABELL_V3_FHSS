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
#include <RF24.h>
#include "RX.h"
#include "Pins.h"
#include <EEPROM.h>
#include "Rx_Tx_Util.h"
#include "SUM_PPM.h"
#include "MyServo.h"    // hacked to remove timer1 ISR - must call this in my own ISR
#include <ClickEncoder.h>    // use liubrary from github https://github.com/soligen2010/encoder

RF24 radio(RADIO_CE_PIN,RADIO_CSN_PIN);
  
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

uint8_t radioChannel[CABELL_RADIO_CHANNELS];

volatile uint8_t currentOutputMode = 255;    // initialize to an unused mode
volatile uint8_t nextOutputMode = 255;       // initialize to an unused mode

#ifdef USE_IRQ_FOR_READ
  volatile bool packetReady = false;
  volatile static unsigned long lastRadioPacketeRecievedTime = 0;
  volatile static unsigned long nextAutomaticChannelSwitch =0;

#endif

MyServo channelServo[RX_NUM_CHANNELS];

ClickEncoder* setFailSafeButton;

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
    Serial.println ("Bind Mode ");
    digitalWrite(LED_PIN, HIGH);      // Turn on LED to indicate bind mode
    radioNormalRxPipeID = 0x01<<43;   // This is a number bigger than the max possible pipe ID, which only uses 40 bits.  This makes sure the bind routine writes to EEPROM
  }
  else
  {
    bindMode = false;
    radioPipeID = radioNormalRxPipeID;    
  }

  setFailSafeButton = new ClickEncoder(BIND_BUTTON_PIN,false);   // use bind pin to check for click to set failsafe values
  setFailSafeButton->setButtonHeldEnabled(true);
  setFailSafeButton->setDoubleClickEnabled(false);
 
  getChannelSequence (radioChannel, CABELL_RADIO_CHANNELS, radioPipeID);

  radio.begin();
  radio.enableDynamicPayloads();
  radio.setPALevel(RADIO_PA_LEVEL);
  radio.setChannel(0);                    // start out on a channel we dont use so we dont start recieving packets yet.  It will get changed when the looping starts
  setNewDataRate();
  radio.setAutoAck(0);

  #ifdef USE_IRQ_FOR_READ
    radio.maskIRQ(true,true,true);         // Mask all interuupts.  RX interrupt (the only one we use) gets turned on after channel change
    pinMode(RADIO_IRQ_PIN,INPUT_PULLUP);
    
    //setup pin change interrupt
    cli();    // switch interrupts off while messing with their settings  
    PCICR =0x02;          // Enable PCINT1 interrupt
    PCMSK1 = RADIO_IRQ_PIN_MASK;
    sei();
  #endif
  
  radio.openReadingPipe(1,radioPipeID);
  radio.startListening();

  outputFailSafeValues(false);   // initialize default values for output channels
  
  Serial.print("Radio ID: ");Serial.print((uint32_t)(radioPipeID>>32)); Serial.print("    ");Serial.println((uint32_t)((radioPipeID<<32)>>32));
  Serial.print("Current Model Number: ");Serial.println(currentModel);

  #ifdef USE_IRQ_FOR_READ
    nextAutomaticChannelSwitch = micros() + ((CABELL_RADIO_CHANNELS+2) * 3000);
  #endif
}

#ifdef USE_IRQ_FOR_READ
//--------------------------------------------------------------------------------------------------------------------------
ISR(PCINT1_vect) {
  if (!digitalRead(RADIO_IRQ_PIN))  {  // pulled low when packet is recieved
    packetReady = true;
    lastRadioPacketeRecievedTime = micros();   //Use this time to calculate the next expected channel when we miss packets
    //Serial.println(lastRadioPacketeRecievedTime);
    nextAutomaticChannelSwitch = lastRadioPacketeRecievedTime + INITIAL_PACKET_TIMEOUT; 

  }
}
#endif

//--------------------------------------------------------------------------------------------------------------------------
void outputChannels() {
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
    outputPWM();                               // Do this first so we have something to send  when PPM enabled
    if (firstPacketOnMode) {                   // First time through attach pins to start output
      attachServoPins();
    }
  }

  if (nextOutputMode == 1) {
    outputSumPPM();                           // Do this first so we have something to send  when PPM enabled
    if (firstPacketOnMode) {
      if (!PPMEnabled()) {
        ppmSetup(PPM_OUTPUT_PIN, RX_NUM_CHANNELS);
      }
    }
  }
  currentOutputMode = nextOutputMode;
}

//--------------------------------------------------------------------------------------------------------------------------
void setNextRadioChannel(bool sendTelemetry) {
  static int currentChannel = CABELL_RADIO_MIN_CHANNEL_NUM;  // Initializes the channel sequence.
  
  #ifdef USE_IRQ_FOR_READ
    //radio.maskIRQ(true,true,true);         // Mask all interuupts.  RX interrupt (the only one we use) gets turned on after channel change
  #endif
  radio.stopListening();
  radio.closeReadingPipe(1);
  if (sendTelemetry) {
    sendTelemetryPacket();
  }
  currentChannel = getNextChannel (radioChannel, CABELL_RADIO_CHANNELS, currentChannel);
  radio.setChannel(currentChannel);
  radio.openReadingPipe(1,radioPipeID);
  radio.startListening();
  #ifdef USE_IRQ_FOR_READ
    //radio.flush_rx();  
    radio.maskIRQ(true,true,false);         // Turn on RX interrupt
  #endif
}

//--------------------------------------------------------------------------------------------------------------------------
bool getPacket() {  
  static unsigned long lastPacketTime = 0;  
  static bool inititalGoodPacketRecieved = false;
  #ifndef USE_IRQ_FOR_READ
    static unsigned long lastRadioPacketeRecievedTime = 0;
    static unsigned long nextAutomaticChannelSwitch = micros() + ((CABELL_RADIO_CHANNELS+2) * 3000);
  #endif
  bool goodPacket_rx = false;

  // process bind button to see if it was pressed, whicn indicates to save failsafe data
  // normally this is done in a timer, but doing it in the loop so it doesn't interfere with the pin change timer.
  setFailSafeButton->service();
  
  // Wait for the radio to get a packet, or the timeout for the current radio channel occurs
  #ifdef USE_IRQ_FOR_READ
  if (!packetReady) {
  #else
  if (!radio.available()) {
  #endif
    if ((long)(micros() - nextAutomaticChannelSwitch) >= 0 ) {      // if timed out the packet eas missed, go to the next channel
      setNextRadioChannel(false);                                   // don't send telemetry when packet missed
      //Serial.println("miss");
      if ((long)(nextAutomaticChannelSwitch - lastRadioPacketeRecievedTime) > RESYNC_TIME_OUT) {  // if a long time passed, increase timeout duration to re-sync with the TX
        nextAutomaticChannelSwitch += RESYNC_WAIT_MICROS;
        Serial.println("Re-sync Attempt");
        setNewDataRate();                       //Alternate data rates until signal found
      } else {
        nextAutomaticChannelSwitch += EXPECTED_PACKET_INTERVAL;
//        Serial.print("\t\tMiss "); Serial.println(currentChannel);
      }
      if (inititalGoodPacketRecieved) {      // only do failsafe and disarms after good communication has been established for this model
        checkFailsafeDisarmTimeout(lastPacketTime);   // at each timeout, check for failsafe and disarm.  When disarmed TX must send min throttle to re-arm.
      }
    }
    #ifndef USE_IRQ_FOR_READ
      delayMicroseconds(20);   // The RF24 library has built in 5uS delay to limit SPI poling.  Adding more delay as there are some indications that too much SPI activity could casue issues. 
                               // the library's 5uS is likely enough, but since it doesn't need to be that fasr in this use case, slow it down just in case it may help reliability.
    #endif
  }  else {
    #ifdef USE_IRQ_FOR_READ
      packetReady = false;
    #else  
      lastRadioPacketeRecievedTime = micros();   //Use this time to calculate the next expected channel when we miss packets
      //Serial.println(lastRadioPacketeRecievedTime);
      nextAutomaticChannelSwitch = lastRadioPacketeRecievedTime + INITIAL_PACKET_TIMEOUT; 
    #endif
    goodPacket_rx = readAndProcessPacket();
    if (goodPacket_rx) {
      inititalGoodPacketRecieved = true;
      lastPacketTime = millis();
      failSafeDisplayFlag = true;
    }
  }
  return goodPacket_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
void checkFailsafeDisarmTimeout(unsigned long lastPacketTime) {
  unsigned long holdMillis = millis();
  
  if ((long)(holdMillis - lastPacketTime)  >  RX_CONNECTION_TIMEOUT) {  
    outputFailSafeValues(true);
  }

  if ((long)(holdMillis - lastPacketTime) >  RX_DISARM_TIMEOUT) { 
    if (throttleArmed) {
      Serial.println("Disarming throttle");
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
  //Serial.println(millis()); 
}

//--------------------------------------------------------------------------------------------------------------------------
void outputFailSafeValues(bool callOutputChannels) {

  loadFailSafeDefaultValues();

  //Serial.println("FS Vals");
  for (uint8_t x =0; x < CABELL_NUM_CHANNELS; x++) {
    channelValues[x] = failSafeChannelValues[x];
    //Serial.println(channelValues[x]);
  }
  
  if (failSafeDisplayFlag && millis() > 1000) {  //startup failsave lasts for first second, so don't display until after that
    Serial.println(F("Failsafe"));
    failSafeDisplayFlag = false;
  }
  
  if (callOutputChannels)
    outputChannels();
}

//--------------------------------------------------------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect){
  if (currentOutputMode == 0)
    MyServoInterruptOneProcessing();
  else if (currentOutputMode == 1)
    SUM_PPM_ISR();
}

//--------------------------------------------------------------------------------------------------------------------------
void unbindReciever() {
  // Reset all of flash memory to unbind reciever
  uint8_t value = 0xFF;
  Serial.print("Overwriting flash with value ");Serial.println(value, HEX);
  for (int x = 0; x < 1024; x++) {
        EEPROM.put(x,value);
  }
  
  Serial.println("Reciever un-bound.  Reboot to enter bind mode");
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
    Serial.print("Bound to new TX address for model number ");Serial.println(modelNum);
    digitalWrite(LED_PIN, LOW);                              // Turn off LED to indicate sucessful bind
    EEPROM.put(softRebindFlagEEPROMAddress,(uint8_t)DO_NOT_SOFT_REBIND);
    setFailSafeDefaultValues();
    outputFailSafeValues(true);
    Serial.println("Reciever bound.  Reboot to enter normal mode");
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
  static unsigned long lastFailSafeSave = 0;
  if ((long)(millis() - lastFailSafeSave) > 200) {          // only save failsafe every 200 ms so that EEPROM is not written too often       
    for (int x = 0; x < CABELL_NUM_CHANNELS; x++) {
      failSafeChannelValues[x] = newFailsafeValues[x];
    }
    failSafeChannelValues[THROTTLE_CHANNEL] = CHANNEL_MIN_VALUE;           // Throttle should always be the min value when failsafe}
    EEPROM.put(failSafeChannelValuesEEPROMAddress,failSafeChannelValues);  
    Serial.println("Fail Safe Values Set");
    lastFailSafeSave = millis();
  }
}

//--------------------------------------------------------------------------------------------------------------------------
bool validateChecksum(CABELL_RxTxPacket_t packet, uint8_t maxPayloadValueIndex) {
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
  bool packet_rx = false;
  CABELL_RxTxPacket_t RxPacket;
  uint16_t tempHoldValues [CABELL_NUM_CHANNELS]; 
  
  radio.read( &RxPacket,  sizeof(RxPacket) );

  setNextRadioChannel((RxPacket.RxMode==CABELL_RxTxPacket_t::RxMode_t::normalWithTelemetry)?true:false);                   // Send telemetry if in telemetry mode.  Doing this as soon as possible to keep timing as tight as possible
  
  uint8_t channelReduction = constrain((RxPacket.option & CABELL_OPTION_MASK_CHANNEL_REDUCTION),0,CABELL_NUM_CHANNELS-CABELL_MIN_CHANNELS);  // Must be at least 4 channels, so cap at 12
  uint8_t packetSize = sizeof(RxPacket) - ((((channelReduction - (channelReduction%2))/ 2)) * 3);      // reduce 3 bytes per 2 channels, but not last channel if it is odd
  uint8_t maxPayloadValueIndex = sizeof(RxPacket.payloadValue) - (sizeof(RxPacket) - packetSize);
  uint8_t channelsRecieved = CABELL_NUM_CHANNELS - channelReduction; 
  

  // Remove 8th bit from RxMode becasye this is a toggle bit that is not included in the checksum
  // This toggle with each xmit so consecrutive payloads are not identical.  This is a work around for a reported bug in clone NRF24L01 chips that mis-took this case for a re-transmit of the same packet.
  uint8_t* p = reinterpret_cast<uint8_t*>(&RxPacket.RxMode);
  *p &= 0x7F;  //ensure 8th bit is not set.  This bit is not included in checksum

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
    Serial.println("RX Pckt Err");
    setNextRadioChannel(false);                   // Dont send telemetry if the packet was in error
  }  

  calculateRSSI(packet_rx);   // This will maintain the RSSI based on if the packet was good or not.
  return packet_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
bool processRxMode (uint8_t RxMode, uint8_t modelNum, uint16_t tempHoldValues[]) {
  bool packet_rx = true;
  static bool failSafeValuesHaveBeenSet = false;

  // fail safe settings can come in on a failsafe packet, but also use a normal packed if bind mode button is pressed after start up
  if (!bindMode && (setFailSafeButton->getButton() == ClickEncoder::Held)) { 
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
                                                    Serial.println("Bind command detected but reciever not in bind mode");
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
                                                         Serial.println("Wrong Model Number");
                                                       }
                                                       break;
                                                      
    case CABELL_RxTxPacket_t::RxMode_t::normalWithTelemetry : 
    case CABELL_RxTxPacket_t::RxMode_t::normal :  if (modelNum == currentModel) {
                                                    digitalWrite(LED_PIN, LOW);
                                                    failSafeValuesHaveBeenSet = false;             // Reset when not in setFailSafe mode so next time failsafe is to be set it will take
                                                    if (!throttleArmed && (tempHoldValues[THROTTLE_CHANNEL] <= CHANNEL_MIN_VALUE)) { 
                                                      Serial.println("Throttle Armed");
                                                      throttleArmed = true;
                                                    }
                                                  } 
                                                  else 
                                                  {
                                                    packet_rx = false;
                                                    Serial.println("Wrong Model Number");
                                                  }
                                                  break;
                                                  
    case CABELL_RxTxPacket_t::RxMode_t::unBind :  if (modelNum == currentModel) {
                                                    unbindReciever();
                                                  } else {
                                                    packet_rx = false;
                                                    Serial.println("Wrong Model Number");
                                                 }
                                                  break;
    default : Serial.println("Unknown RxMode");
              packet_rx = false;
              break;          
  }
  return packet_rx;  
}

//--------------------------------------------------------------------------------------------------------------------------
bool decodeChannelValues(CABELL_RxTxPacket_t RxPacket, uint8_t channelsRecieved, uint16_t tempHoldValues[]) {
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
//  Serial.println(millis());
  
  return packet_rx;
}

//--------------------------------------------------------------------------------------------------------------------------
void setNewDataRate() {
  static bool fastDataRate = true;

  fastDataRate = !fastDataRate;

  if (fastDataRate) {
    radio.setDataRate(RF24_1MBPS);
  } else {
    radio.setDataRate(RF24_250KBPS );
  }  
}

//--------------------------------------------------------------------------------------------------------------------------
void sendTelemetryPacket() {
  radio.openWritingPipe(radioPipeID);

  uint8_t sendPacket[4] = {CABELL_RxTxPacket_t::RxMode_t::telemetryResponse};
 
  static int8_t packetCounter = 0;  // this is only used for toggleing bit
  packetCounter++;
  sendPacket[0] &= 0x7F;               // clear 8th bit
  sendPacket[0] |= packetCounter<<7;   // This causes the 8th bit of the first byte to toggle with each xmit so consecrutive payloads are not identical.  This is a work around for a reported bug in clone NRF24L01 chips that mis-took this case for a re-transmit of the same packet.
  sendPacket[1] = calculateRSSI(false);  // Passing false will return the RSSI without affecting the RSSI calculation.  Another call to calulateRSSI elsehere will pass true for a good packet to maintain the RSSI value.
  
  sendPacket[3] = analogRead(TELEMETRY_ANALOG_INPUT_1)/4;  // Send a 8 bit value (0 to 255) of the analog input.  Can be used for Lipo coltage or other analog input for telemetry
  sendPacket[4] = analogRead(TELEMETRY_ANALOG_INPUT_2)/4;  // Send a 8 bit value (0 to 255) of the analog input.  Can be used for Lipo coltage or other analog input for telemetry

  uint8_t packetSize =  sizeof(sendPacket);
  radio.startFastWrite( &sendPacket[0], packetSize, 0); 
      // calculate transmit time based on packet size and data rate of 1MB per sec
      // This is done becasue polling the status register during xmit to see when xmit is done casued issues some times.
      // bits = packstsize * 8  +  73 bits overhead
      // at 1 MB per sec, one bit is 1 uS
      // then add 160 uS which is 130 uS to begin the xmit and 30 uS fudge factor
      delayMicroseconds(((unsigned long)packetSize * 8ul)  +  73ul + 160ul)   ;
}


//--------------------------------------------------------------------------------------------------------------------------
uint8_t calculateRSSI(bool goodPacket) {
  // Passign in true for a good packet will improve the RSSI in each interval.
  // This can additionally be called at any time with false to just return the current RSSI and it wont affect calculation (but may trigger the interval end logic) 

  static uint8_t rssi = TELEMETRY_RSSI_MAX_VALUE;                                           // Initialize to perfect RSSI until it can be calcualted
  static unsigned long nextRssiCalcTime = micros() + TELEMETRY_RSSI_CALC_INTERVAL;
  static uint16_t rssiCounter = 0;

  if (goodPacket)
    rssiCounter++;
  
  if ((long)(micros() - nextRssiCalcTime) >= 0 ) {      // Interval end
    // RSSI is the based on hte expected packet rate for the interval vs. actiual packets.  255 is 100% of the expected rate.
    nextRssiCalcTime = micros() + TELEMETRY_RSSI_CALC_INTERVAL;
    // calculate rssi as a ratio of expected to actual packets as a percent then map to actual range
    uint16_t rssiCalc = (uint16_t) (((float)rssiCounter / (float)((uint16_t)((float)TELEMETRY_RSSI_CALC_INTERVAL/(float)EXPECTED_PACKET_INTERVAL)) * (float) (TELEMETRY_RSSI_MAX_VALUE - TELEMETRY_RSSI_MIN_VALUE)) + float(TELEMETRY_RSSI_MIN_VALUE));
    rssi = constrain(rssiCalc,TELEMETRY_RSSI_MIN_VALUE,TELEMETRY_RSSI_MAX_VALUE);
//    Serial.print(rssiCounter);Serial.print(' ');
//    Serial.println(rssi);
    rssiCounter = 0;
  }
  return rssi;
}








