# RC_RX_CABELL_V3_FHSS
## Background
RC_RX_CABELL_V3_FHSS is an open source receiver for remote controlled vehicles.  Developed by Dennis Cabell (KE8FZX)
The hardware for this receiver is an Arduino Pro Mini (using a 5V ATMEGA328P at 16 Mhz) and one or two NRF24L01+ modules.  Both are inexpensively available on-line.  Be sure to get the version of a Pro Mini that has pins A4, A5, A6 and A7 broken out.

~~The transmitter side of this RC protocol is in the Multi-protocol TX Module project at [https://github.com/pascallanger/DIY-Multiprotocol-TX-Module]( https://github.com/pascallanger/DIY-Multiprotocol-TX-Module).~~
Transmitter code has not yet been merged into Multiprotocol.  To try this out, please use the soligen2010 fork of MultiProtocol at [https://github.com/soligen2010/DIY-Multiprotocol-TX-Module]( https://github.com/soligen2010/DIY-Multiprotocol-TX-Module).

## The Protocol
The protocol used is named CABELL_V3 (the third version, but the first version publicly released).  It is a FHSS protocol using the NRF24L01+ 2.4 GHz transceiver.  45 channels are used from 2.403 through 2.447 GHz.  The reason for using 45 channels is to keep operation within the overlap area between the 2.4 GHz ISM band (governed in the USA by FCC part 15) and the HAM portion of the band (governed in the USA by FCC part 97).  This allows part 15 compliant use of the protocol, while allowing licensed amateur radio operators to operate under the less restrictive part 97 rules if desired.

Each transmitter is assigned a random ID (this is handled by the Multi-protocol TX Module) based on this ID one of 362880 possible channel sequences is used for the frequency hopping.  The hopping pattern algorithm ensures that each hop moves at least 9 channels from the previous channel.  One packet is sent every 3 - 4 milliseconds (depending on options chosen), changing channels with each packet. All 45 channels are used equally.

The CABELL_V3 protocol can be configured to send between 4 and 16 RC channels, however this receiver software is currently only capable of outputting up to 8 channels via PWN or PPM.  This is because only 8 channels were conveniently laid out on the Arduino Pro Mini, and more than 8 channels over PPM would be too slow.  SBUS output allows all 16 channels to be used.

I recommend reducing the number of channels as much as possible based on what your model requires.  Fewer channels will use a smaller packet size, which improves transmission reliability (fewer bytes sent means less opportunity for errors).

The protocol also assigns each model a different number so one model setting does not control the wrong model.  The protocol can distinguish up to 255 different models, but be aware that the multiprotocol transmitter software only does 16.

## Hardware
A 5V 16Mhz Arduino Pro Mini and one or 2 NRF24L01+ modules are needed for this receiver.  Be sure to get the version of a Pro Mini that has pins A4 and A5 broken out (and A6, A7 too for telemetry analog inputs).  The hardware folder contains a schematic and a PCB layout using a single transceiver module for PWN or PPM output; however, this version of the PCB has not been tested.  I am still using the previous version of the PCB and modifying it to implement the changes included in the new version. If anyone tries this PCB version, please open an issue let me know how it works. 

There is also a schematic for using 2 NRF24L01 modules for diversity.  If anyone designs a board for this, please contribute it.  This schematic also has the signal inverter needed for SBUS.

The PA+LNA versions of the NRF24L01+ module provide more receiver range and reliability than the non PA+LNA versions, However the less expensive modules also work OK if the on-board antenna is replaced with a better antenna, although randomly some units seem to work well and others not so well.  I recommend range testing each module.  These modules are also typically unshielded, but work better when shielded.  Here is an outline of the modifications (TODO: add pictures)

#### Antenna
* For the PA_LNA module that has an SMA Connector, remove the connector. A small butane torch to heat the connector works well.  When the solder melts, it pulls right off 
* Cut the antenna trace.  On the PA+LNA module cut the trace back far enough so that no portion of the trace extends beyond the ground plane on the back of the board.  For the boards with an antenna, cut the trace leading to the antenna shortly after the last surface mount component (a capacitor, I think).
* Scrape or sand away the solder mask over the antenna trace.  On the boards with an antenna you will also need to sand away the area next to the trace for a ground connection.  For the PA+LNA module you can re-use the ground connections that were used for the connector.
* Tin the ground and antenna attachment points with solder
* Strip back the coax and separate the braid, twisting it into a wire that can be soldered to the ground point. Tin this.
* Strip the center conductor back to within 1mm of where the braided part now ends.  Tin this.
* Solder the coax to the board.  Use hot glue to fix the coax to the board as strain relief.
* On the other end of the coax, strip back and remove the braided ground conductor to leave the center wire exposed to form the antenna. Leave the inner wire insulation on.

I am no expert, but based on the best understand I have been able to achieve on the theory, using RG-178 or RG-316 (both have speed factor of 69.5) the calculated theoretical coax lengths to use are:
* For the shielded portion of the coax, ideally make the length a multiple of 42.5 mm.  170mm seems like a good length. 
* The length of the antenna at the end should theoretically be 32.5mm (based on center frequency of 2.425 GHz.  I don’t have the proper equipment to verify these lengths, experiment for what works best for you.  My tests showed 32mm seems to work better than 32.5 for a monopole antenna.

The antennas that come with the PA/LNA modules on EBay are often not very good.  Get a good antenna for your transmitter.  For the transmitter I use a 5dbi antenna constructed as recommend by Andrew McNeil at [https://www.youtube.com/watch?v=bs8hvXGJdhM](https://www.youtube.com/watch?v=bs8hvXGJdhM).  Good RX and TX antennas make a big difference to performance.

#### Shielding
Shielding will require using 2 layers of heat shrink tubing around the whole module.  

* First solder a bare wire onto a ground point on the board.
* Encase as much of the board as possible in heat shrink tubing, leaving the antenna and ground wire sticking out, and don’t cover the pins.
* Use copper tape to wrap the module as much as possible on the heat shrink tubing, while staying away from the edge of the heat shrink. (Aluminum  tape can work too, but you cant solder to it.)
* Bend the ground wire back over the copper tape and solder it to the copper tape.
* Encase as much of the board as possible again in heat shrink tubing.  Don’t cover the pins. Be sure to cover all the ground wire and copper tape shielding.

These shielding steps are based partially  in the article at [http://blog.blackoise.de/2016/02/fixing-your-cheap-nrf24l01-palna-module/](http://blog.blackoise.de/2016/02/fixing-your-cheap-nrf24l01-palna-module/)

## Receiver Output
The output method is controlled via the option byte in the protocol header.  See the Taranis Setup section for  more details.
* Sum PPM on Arduino pin D2 (the Roll pin). The channel sequence is AETR, more specifically Roll, Pitch, Throttle, Yaw, AUX1, AUX2, AUX3, AUX4.
* Individual PWM channel output on pins D2 through D9 in the sequence Roll, Pitch, Throttle, Yaw, AUX1, AUX2, AUX3, AUX4.
* SBUS (__Experimental__) output on the TX pin through an inverter.  The channel sequence is AETR, more specifically Roll, Pitch, Throttle, Yaw, AUX1, AUX2, AUX3, AUX4, AUX5, AUX6, AUX7, AUX8, AUX9, AUX10, AUX11, AUX12.  SBUS packets are output every 7 milliseconds.

## Receiver Setup
The receiver must be bound to the transmitter. There are several ways for the receiver to enter Bind Mode:
* A new Arduino will start in bind mode automatically.  Only an Arduino that was flashed for the first time (not previously bound) does this.  Re-flashing the software will retain the old binding unless the EEPROM has been erased.
* Erasing the EEPROM on the Arduino will make it start up in bind mode just like a new Arduino. The Arduino sketch [here]( https://github.com/soligen2010/Reset_EEPROM) will erase the EEPROM.
* Connect the Bind Jumper, or press the Bind button while the receiver powers on.
* The protocol has a Un-bind command (it erases the EEPROM), after which a re-start will cause the receiver to enter bind mode just like a new Arduino. After an Unbind the LED will blink until the receiver is re-started.

Turn on the transmitter in bind mode.  The LED turns on solid, then will blink slowly after a successful bind.  Re-start the receiver after the bind and take the transmitter out of Bind mode, then test the connection.

## Fail-safe
The receiver fail-safes after 1 second when no packets are received.  If a connection is not restored within 3 seconds then the receiver will disarm.  
* At fail-safe, the throttle is set to minimum, and the other channels are set to the failsafe value.
* After disarm, the throttle will stay at minimum until the receiver is re-armed.  To re-arm the receiver move the throttle to the minimum position.

When a receiver is bound the failsafe values are reset to the default values, which are throttle minimum and all other channels at mid-point.

## Customizing Fail-safe Values
__Do not set fail-safe values while in flight.__  Due to the length of time it takes to write the new fail-safe values to EEPROM, the receiver may go into fail-safe mode while saving the values, causing loss of control of the model.  Before flying a model, always test the fail-safe values after they have been set.

Fail-safe set mode will set the fail-safe values.  This can be done one of two ways:
* A set-Fail-Safe packet can be sent from the transmitter.  The values from the first packet in a series for set-Fail-Safe packets are saved as the new fail-safe values.  The LED is turned on when a set-Fail-Safe packet is received, and stays on as long as set-Fail-Safe packets continue to be received.  The LED is turned off when set-Fail-Safe values stop being received
* After the receiver has initialized, the bind button (or bind jumper) can be held for one to 2 seconds until the LED is turned on.  The values from the first packet received after the LED is turned on will be saved as the new fail-safe values.  The LED will turn off when the button is released (or jumper removed).

When fail-safe set mode is entered, the LED is turned on and stays on until the failsafe set mode is exited.  Only the values from the first packet received in fail-safe set mode are saved (this is to avoid accidentally using up all of the EEPROMs limited number of write operations)

Values for all channels can be set except for the throttle channel.  The fail-safe for throttle is always the minimum throttle.

## Diversity
Diversity is achieved by using 2 NRF24L01 modules, which improves link reliability. Try to orient the antennas close to 90 degrees to each other so at least one antenna has a good orientation to the transmitter antenna.  The use of a second NRF24L01 module for diversity is optional.  The code automatically detects if one or 2 modules are connected.  See the "with diversity" schematic in the hardware folder for how to wire the NRF24L01 modules.

Both modules listen for an incoming packet.  If the primary receiver does not get a packet when expected, the secondary receiver is checked for the missed packet.  After each packet, the primary/secondary receivers are swapped, except in the case where only the primary receiver had the packet, in which case this receiver will retain the primary role.  Telemetry packets are transmitted using the same receiver that was used to read the packet.  If both receivers got the incoming packet, then the packet in the secondary receiver is discarded.  The net effect is that the receivers alternate with each packet, except when only one receiver is receiving, in which case that receiver continues to be used for both the primary receiver and telemetry transmit.

## Taranis Setup using Multi-Protocol Module
Needs work

## Telemetry

When the sub-protocol is set to Normal with Telemetry, the receiver sends telemetry packets back to the transmitter.  Three values are returned, a simulated RSSI, and the voltages on the Arduino pins A6 and A7.  A receiver module with diversity is recommended when using telemetry to increase the reliability of the telemetry packets being received by the transmitter.

### RSSI

Because the NRF24L01 does not have an RSSI feature, the RSSI value is simulated based on the packet rate.  The base of the RSSI calculation is the packet success rate from 0 to 100.  This value is re-calculated approximately every 1/2 second (every 152 expected packets).  This success percentage is then modified in real time based on packets being missed, so that if multiple packets in a row are missed the RSSI value drops without having to wait for the next re-calculation of the packet rate.

In practice, the packet rate seems to stay high for a long range, then drop off quickly as the receiver moves out of range.  Typically, the telemetry lost warning happens before the RSSI low warning.

The RSSI class encapsulates the RSSI calculations. If you are so inclined, feel free play with the calculation. If anyone finds an algorithm that works better, please contribute it. 

### Analog Values

Analog values are read on Arduino pins A6 and A7.  Running on a, Arduino with VCC of 5V, only values up to 5V can be read.  __A value on A6 or A7 over the Arduino VCC will cause  damage__, so care must be taken to ensure the voltage is in a safe range.

Values from pins A6 and A7 come into a Taranis transmitter as telemetry values A1 and A2.  You can use either of these to read battery voltage or the output of current sensor.  The following article explains how to input battery voltage to A2 on an Frsky receiver using a voltage divider.  The same method can be used to read battery voltage on this receiver.  [http://olex.biz/tips/lipo-voltage-monitoring-with-frsky-d-receivers-without-sensors](http://olex.biz/tips/lipo-voltage-monitoring-with-frsky-d-receivers-without-sensors).

The values sent are 0 - 255 corresponding to 0V - 5V.  This will need to be re-scaled to the actual voltage (or current, etc.) in the transmitter on the telemetry  configuration screen.

## Packet Format

```
typedef struct {
   enum RxMode_t : uint8_t {   // Note bit 8 is used to indicate if the packet is the first of 2 on the channel.  
                               // Mask out this bit before using the enum
         normal                 = 0,  // 250 kbps
         bind                   = 1,
         setFailSafe            = 2,
         normalWithTelemetry    = 3,  
         telemetryResponse      = 4,
         unBind                 = 127
   } RxMode;
   uint8_t  reserved = 0;
   uint8_t  option;
                          /*   mask 0x0F    : Channel reduction.  The number of channels to not send (subtracted from the 16 max channels) at least 4 channels are always sent.
                           *   mask 0x30>>4 : Receiver output mode
                           *                  0 (00) = Single PPM on individual pins for each channel 
                           *                  1 (01) = SUM PPM on channel 1 pin
                           *                  2 (10) = SBUS output (Experimental)
                           *                  3 (11) = Unused
                           *   mask 0x40>>6   Contains max power override flag for Multiprotocol TX module. Also sent to RX
                           *                  The RX uses MAX power when 1, HIGH power when 0
                           *   mask 0x80>>7   Unused 
                           */  
   uint8_t  modelNum;
   uint8_t  checkSum_LSB;   // Checksum least significant byte
   uint8_t  checkSum_MSB;   // Checksum most significant byte
   uint8_t  payloadValue [24] = {0}; //12 bits per channel value, unsigned
} CABELL_RxTxPacket_t;   
```
Each 12 bits in payloadValue is the value of one channel.  The channel order is EART (i.e. Pitch, Roll, Yaw, Throttle, AUX1, AUX2, AUX3, AUX4).  Valid values are in the range 1000 to 2000.  The values are stored big endian.

Using channel reduction reduces the number of bytes sent, thereby trimming off the end of the payloadValue array.

## Credits
davidbuzz on github.  Although none of his code is in this project, my very first foray into DIY rx/tx was based on his esp8266_wifi_tx project.

iforce2d on YouTube.  Although none of his code is in this project, the work he did with the NRF24L01 was a big inspiration.

All the contributors to the pascallanger/DIY-Multiprotocol-TX-Module project on GitHub.  This is a great contribution to the RC community and houses the TX side of this protocol.  

All the contributors to the nRF24/RF24 project on GitHub.  This library was essential to learning to use the NRF24L01.

All the contributors to Arduino Core and standard libraries.  Especially the Servo library, which I modified to adapt to the needs of this project.

The author of the PPM algorithm at https://code.google.com/archive/p/generate-ppm-signal/ I believe this to be david.hasko94@gmail.com

The channel sequence is generated using a permutation algorithm described at http://stackoverflow.com/questions/7918806/finding-n-th-permutation-without-computing-others 

Nick Gammon for his tutorials on interrupts and async reading of ADC pins at https://www.gammon.com.au/adc

My wife, for tolerating the obsession I have had with this project.

## License Info
Copyright 2017 by Dennis Cabell
KE8FZX
 
To use this software, you must adhere to the license terms described below, and assume all responsibility for the use of the software.  The user is responsible for all consequences or damage that may result from using this software. The user is responsible for ensuring that the hardware used to run this software complies with local regulations and that any radio signal generated from use of this software is legal for that user to generate.  The author(s) of this software assume no liability whatsoever.  The author(s) of this software is not responsible for legal or civil consequences of using this software, including, but not limited to, any damages cause by lost control of a vehicle using this software.   If this software is copied or modified, this disclaimer must accompany all copies.
 
This project is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

RC_RX_CABELL_V3_FHSS is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with RC_RX_CABELL_V3_FHSS.  If not, see <http://www.gnu.org/licenses/>.