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
 
#include <arduino.h>
#include "Rx_Tx_Util.h"
#include "RX.h"


//--------------------------------------------------------------------------------------------------------------------------
uint8_t getNextChannel (uint8_t seqArray[], uint8_t seqArraySize, uint8_t prevChannel) {
  /* Possible channels are in 5 bands, each band comprised of seqArraySize channels
   * seqArray contains seqArraySize elements in the relative order in which we should progress through the band 
   * 
   * Each time the channel is changes, bands change in a way so that the next channel will be in a
   * different non-adjacent band. Both the band changes and the index in seqArray is incremented.
   */
  prevChannel -= CABELL_RADIO_MIN_CHANNEL_NUM;                             // Subtract CABELL_RADIO_MIN_CHANNEL_NUM becasue it was added to the return value
  prevChannel = constrain(prevChannel,0,(seqArraySize * 5)     );    // Constrain the values just in case something bogus was sent in.
  
  uint8_t currBand = prevChannel / seqArraySize;             
  uint8_t nextBand = (currBand + 3) % 5;

  uint8_t prevChannalSeqArrayValue = prevChannel % seqArraySize;
  uint8_t prevChannalSeqArrayPosition = 0;
  for (int x = 0; x < seqArraySize; x++) {                    // Find the position of the previous channel in the array
    if (seqArray[x] == prevChannalSeqArrayValue) {
      prevChannalSeqArrayPosition = x;
    }
  }
  uint8_t nextChannalSeqArrayPosition = prevChannalSeqArrayPosition + 1;
  if (nextChannalSeqArrayPosition >= seqArraySize) nextChannalSeqArrayPosition = 0;

  return (seqArraySize * nextBand) + seqArray[nextChannalSeqArrayPosition] + CABELL_RADIO_MIN_CHANNEL_NUM;   // Add CABELL_RADIO_MIN_CHANNEL_NUM so we dont use channel 0 as it may bleed below 2.400 GHz
}

//--------------------------------------------------------------------------------------------------------------------------
void getChannelSequence (uint8_t outArray[], uint8_t numChannels, uint64_t permutation) {
  /* This procedure initializes an array with the sequence progression of channels.
   * This is not the actual channels itself, but the sequence base to be used within bands of 
   * channels.
   * 
   * There are numChannels! permutations for arranging the channels
   * one of these permutations will be calculated based on the permutation input
   * permutation should be between 1 and numChannels! but the routine will constrain it
   * if these bounds are exceeded.  Typically the radio's unique TX ID shouldbe used.
   * 
   * The maximum numChannels is 20.  Anything larget than this will cause the uint64_t
   * variables to overflow, yielding unknown resutls (possibly infinate loop?).  Therefor
   * this routine constrains the value.
   */  
  uint64_t i;   //iterator counts numChannels
  uint64_t indexOfNextSequenceValue;
  uint64_t numChannelsFactorial=1;
  uint8_t  sequenceValue;

  numChannels = constrain(numChannels,1,20);

  for (i = 1; i <= numChannels;i++) {
    numChannelsFactorial *= i;      //  Calculate n!
    outArray[i-1] = i-1;            //  Initialize array with the sequence
  }
  
  permutation = (permutation % numChannelsFactorial) + 1;    // permutation must be between 1 and n! or this algorithm will infinate loop

  //Rearrange the array elements based on the permutation selected
  for (i=0, permutation--; i<numChannels; i++ ) {
    numChannelsFactorial /= ((uint64_t)numChannels)-i;
    indexOfNextSequenceValue = i+(permutation/numChannelsFactorial);
    permutation %= numChannelsFactorial;
    
    //Copy the value in the selected array position
    sequenceValue = outArray[indexOfNextSequenceValue];
    
    //Shift the unused elements in the array to make room to move in the one just selected
    for( ; indexOfNextSequenceValue > i; indexOfNextSequenceValue--) {
      outArray[indexOfNextSequenceValue] = outArray[indexOfNextSequenceValue-1];
    }
    // Copy the selected value into it's new array slot
    outArray[i] = sequenceValue;
    Serial.println(sequenceValue);
  }
}





