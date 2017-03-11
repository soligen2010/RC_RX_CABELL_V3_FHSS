# RC_RX_CABELL_V3_FHSS
## Background
RC_RX_CABELL_V3_FHSS is an open source receiver for remote controlled vehicles.  Developed by Dennis Cabell (KE8FZX)
The hardware for this receiver is an Arduino Pro Mini (using an ATMEGA328P) and an NRF24L01+ module.  Both are inexpensively available on-line.  Be sure to get the version of a Pro Mini that has pins A4 and A5 broken out.

~~The transmitter side of this RC protocol is in the Multi-protocol TX Module project at [https://github.com/pascallanger/DIY-Multiprotocol-TX-Module]( https://github.com/pascallanger/DIY-Multiprotocol-TX-Module).~~
Transmittter code has not yet been merged into Multiprotocol.  TO try this out, plese use the soligen2010 fork of MultiProtocol at [https://github.com/soligen2010/DIY-Multiprotocol-TX-Module]( https://github.com/soligen2010/DIY-Multiprotocol-TX-Module)

## The Protocol
The protocol used is named CABELL_V3 (the third version, but the first version publicly released).  It is a FHSS protocol using the NRF24L01 2.4 GHz transceiver.  45 channels are used from 2.403 through 2.447 GHz.  The reason for using 45 channels it to keep operation within the overlap area between the 2.4 GHz ISM band (governed in the USA by FCC part 15) and the HAM portion of the band (governed in the USA by FCC part 97).  This allows part 15 compliant use of the protocol, while allowing licensed amateur radio operators to operate under the less restrictive part 97 rules if desired.

Each transmitter is assigned a random ID (this is handled by the Multi-protocol TX Module) based on this ID one of 362880 possible channel sequences is used for the frequency hopping.  The hopping pattern algorithm ensures that each hop moves at least 9 channels from the previous channel.  One packet is sent every 3 milliseconds, changing channels with each packet. All 45 channels are used equally.

The CABELL_V3 protocol can be configured to send between 4 and 16 RC channels, however this receiver software is currently only capable of outputting up to 8 channels.  This is because only 8 channels were conveniently laid out on the Arduino Pro Mini, and more than 8 channels over PPM would be too slow.  Future serial output methods will hopefully allow all 16 channels to be used.

I recommend reducing the number of channels as much as possible based on what your model requires.  Fewer channels will use a smaller packet size, which improves transmission reliability (fewer bytes sent means less opportunity for errors).

The protocol also assigns each model a different number so one set of model setting does not control the wrong model.  The protocol can distinguish up to 255 different models, but be aware that the multiprotocol transmitter software only does 16.

## Hardware
An Arduino Pro Mini and an NRF24L01+ module are needed for this receiver.  Be sure to get the version of a Pro Mini that has pins A4 and A5 broken out.  The hardware folder contains a schematic and a PCB layout; however this version of the PCB has not been tested.  I am still using the previous version of the PCB and modifying it to implement the changes included in the new version. If anyone tries this PCB version, please open an issue let me know how it works.

The PA+LNA versions of the NRF24L01+ module provide more receiver range and reliability than the non PA+LNA versions, However the less expensive modules also work well if the on-board antenna is replaced with a better antenna.  These modules are also typically unshielded, but work better when shielded.  Here is an outline of the modifications (TODO: add pictures)

## Antenna
* For the PA_LNA module that has an SMA Connector, remove the connector. A small butane torch to heat the connector work well.  When the solder melts, it pulls right off 
* Cut the antenna trace.  On the PA+LNA module cut the trace back far enough so that no portion of the trace extends beyond the ground plane on the back of the board.  For the boards with an antenna, cut the trace leading to the antenna shortly after the last surface mount component (a capacitor, I think).
* Scrape or sand away the solder mask over the antenna trace.  On the boards with an antenna you will also need to sand away the area next to the trace for a ground connection.  For the PA+LNA module you can re-use the ground connections that were used for the connector.
* Tin the ground and antenna attachment points with solder
* Strip back the coax and separate the braid, twisting it into a wire that can be soldered to the ground point. Tin this.
* Strip the center conductor back to within 1mm of where the braided part now ends.  Tin this.
* Solder the coax to the board.  Use hot glue to fix the coax to the board as strain relief.
* On the other end of the coax, strip back and remove the braided ground conductor to leave the center wire exposed to form the antenna. Leave the inner wire insulation on.

I am no expert, but based on the best understand I have been able to achieve on the theory, using RG-178 or RG-316 (both have speed factor of 69.5) the calculated theoretical lengths to use are:
* For the shielded portion of the coax, ideally make the length a multiple of 42.5 mm.  170mm seems like a good length. 
* The length of the antenna at the end should be 32.5mm (based on center frequency of 2.425 GHz.  I don’t have the proper equipment to verify these lengths, experiment for what works best for you.

## Shielding
Shielding will require using 2 layers of heat shrink tubing around the whole module.  

* First solder a bare wire onto a ground point on the board.
* Encase as much of the board as possible in heat shrink tubing, leaving the antenna and ground wire sticking out, and don’t cover the pins.
* Use copper tape to wrap the module as much as possible on the heat shrink tubing, while staying away from the edge of the heat shrink.
* Bend the ground wire back over the copper tape and solder it to the copper tape.
* Encase as much of the board as possible again in heat shrink tubing.  Don’t cover the pins. Be sure to cover all the ground wire and copper tape shielding.

## Receiver Output
The output method is controlled via the option byte in the protocol header.  There are currently 2 options:
* Sum PPM on Arduino pin D2 (the Roll pin). The channel sequence is AETR, more specifically Roll, Pitch, Throttle, Yaw, AUX1, AUX2, AUX3, AUX4.
* Individual channel output on pins D2 through D9 in the sequence Roll, Pitch, Throttle, Yaw, AUX1, AUX2, AUX3, AUX4.

## Receiver Setup
The receiver must be bound to the transmitter. There are several ways for the receiver to enter Bind Mode:
* A new Arduino will start in bind mode automatically.  Only and Arduino that flashed for the first time does this.  Re-flashing the software will retain the old binding unless the EEPROM has been erased.
* Erasing the EEPROM on the Arduino will make it start up in bind mode just like a new Arduino. The Arduino sketch [here]( https://github.com/soligen2010/Reset_EEPROM) will erase the EEPROM.
* Connect the Bind Jumper, or press the Bind button while the receiver powers on.
* The protocol has a Un-bind command (it erases the EEPROM), after which a re-start will cause the receiver to enter bind mode just like a new Arduino. Still TBD is how to get the Multi-protocol TX Module to send a Un-bind command. After an Unbind the LED will blink until the receiver is re-started.

 Turn on the transmitter in bind mode.  The LED turn on solid, then will blink slowly after a successful bind.  Re-start the receiver after the bind and take the transmitter out of Bind mode, then test the connection.

## Failsafe
The receiver will failsafe after 1 second when no packets are received.  If a connection is not restored within 3 seconds then the receiver will disarm.  
* At fail-safe, the throttle is set to minimum, and the other channels are set to the failsafe value.
* After disarm, the throttle will stay at minimum until the receiver is re-armed.  To re-arm the receiver move the throttle stick to the minimum position.

When a receiver is bound the failsafe values are reset to the default values, which are throttle minimum and all other channels at mid-point.

## Customizing Failsafe Values
__Do not set failsafe values while in flight.__  Due to the length of time it takes to write the new failsafe values to EEPROM, the receiver may go into failsafe mode while saving the values, causing loss of control of the model.  Before flying a model, always test the failsafe values after they have been set.

Failsafe set mode will set the failsafe values.  This can be done one of two ways:
* A setFailSafe packet can be sent from the transmitter.  The values from the first packet in a series for setFailSafe packets are saved as the new failsafe values.  The LED is turned on when a setFailSafe packet is received, and stays on as long as setFailSafe packets continue to be received.  The LED is turned off when setFailSafe values stop being received
* After the receiver has initialized, the bind button (or use bind jumper) can be held for one to 2 seconds until the LED is turned on.  The values from the first packet received after the LED is turned on will be saved as the new failsafe values.  The LED will turn off when the button is released (or jumper removed).

When failsafe set mode is entered the LED is turned on and stays on until the failsafe set mode is exited.  Only the values from the first packet received in failsafe set mode are saved (this is to avoid accidentally using up all of the EEPROMs limited number of write operations)

Values for all channels can be se except for the throttle channel.  The fail safe for throttle is always the minimum throttle.

## Packet Format

```
typedef struct {
   enum RxMode_t : uint8_t {   // Note bit 8 is used to indicate if the packet is the first of 2 on the channel.  
                               // Mask out this bit before using the enum
         normal      = 0,
         bind        = 1,
         setFailSafe = 2,
         unBind      = 127
   } RxMode;
   uint8_t  reserved = 0;
   uint8_t  option;
                          /*   mask 0x0F    : Channel reduction.  The number of channels to not send 
                           *                  (subtracted frim the 16 max channels) at least 4 are always sent
                           *   mask 0x70>>4 : Reciever outout mode
                           *                  1 = Single PPM on individual pins for each channel 
                           *                  2 = SUM PPM on channel 1 pin
                           *   mask 0x80>>7   Unused by RX.  Contains max power override flag
                           */  
   uint8_t  modelNum;
   uint16_t checkSum; 
   uint8_t  payloadValue [24] = {0}; //12 bits per channel value, unsigned
} CABELL_RxTxPacket_t;   
```
Each 12 bits in payloadValue is the value of one channel.  The channel order is EART (i.e. Pitch, Roll, Yaw, Throttle, AUX1, AUX2, AUX3, AUX4).  Valid values are in the range 1000 to 2000.  The values are stored big endian.

Using channel reduction reduces the number of bytes sent, thereby trimming off the end of the payloadValue array.

## License Info
Copyright 2017 by Dennis Cabell
KE8FZX
 
To use this software, you must adhere to the license terms described below, and assume all responsibility for the use of the software.  The user is responsible for all consequences or damage that may result from using this software. The user is responsible for ensuring that the hardware used to run this software complies with local regulations and that  any radio signal generated from use of this software is legal for that user to generate.  The author(s) of this software assume no liability whatsoever.  The author(s) of this software is not responsible for legal or civil consequences of  using this software, including, but not limited to, any damages cause by lost control of a vehicle using this software.   If this software is copied or modified, this disclaimer must accompany all copies.
 
This project is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

RC_RX_CABELL_V3_FHSS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with RC_RX_CABELL_V3_FHSS.  If not, see <http://www.gnu.org/licenses/>.