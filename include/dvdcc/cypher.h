// Copyright (C) 2025     Josh Wood
//
// This portion is very closely based on the unscrambler project
// written by:
//              Victor Muñoz (xt5@ingenieria-inversa.cl)
//              https://github.com/saramibreak/unscrambler
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef DVDCC_CYPHER_H_
#define DVDCC_CYPHER_H_

#include <stdlib.h>

// Class for creating the cypher key used to decode raw DVD data.
class Cypher {

 public:
  Cypher(unsigned int seed, unsigned int length);
  ~Cypher() { free(bytes); };

  void Decode(unsigned char *data, unsigned int start);
  void Decode32(unsigned char *data, unsigned int start);
  void Decode64(unsigned char *data, unsigned int start);

  unsigned int seed;     // seed value used to create the cypher
  unsigned int length;   // cypher length in bytes
  unsigned int length32; // length used for 32 bit decode speedup
  unsigned int length64; // length used for 64 bit decode speedup
  unsigned char *bytes;  // byte values of the generated cypher

}; // END class Cypher()

Cypher::Cypher(unsigned int seed, unsigned int length) : seed(seed), length(length) {
  // Constructor that generates a cypher used to decode raw DVD data.
  //
  // Note:
  //     Cypher generation is implemented as a 15 bit
  //     Linear Feedback Shift Register (LFSR) with
  //     bits 10 and 14 as taps. See:
  //
  // [1] https://en.wikipedia.org/wiki/Linear-feedback_shift_register
  // [2] https://hitmen.c02.at/files/docs/gc/Ingenieria-Inversa-Understanding_WII_Gamecube_Optical_Disks.html
  //
  // Args:
  //     seed (unsigned int): seed value for the cypher construction
  //     length (unsigned int): desired length of the cypher in bytes
  //
  // Returns:
  //     (unsigned char *)

  if (length % 4 != 0) {
    printf("dvdcc:cypher:Cypher::Cypher() Length %d is not a muliple of 4 for Decode32().", length);
    exit(0);
  } else length32 = length / 4;

  if (length % 8 != 0) {
    printf("dvdcc:cypher:Cypher::Cypher() Length %d is not a muliple of 8 for Decode64().", length);
    exit(0);
  } else length64 = length / 8;

  // local variables for loop
  unsigned int n, bit;

  // initialize the shift register
  unsigned int lfsr = seed;

  // allocate space for cypher bytes
  bytes = (unsigned char *) malloc(length);

  // loop to calculate bytes by shifting lfsr and
  // then updating lfsr using taps at bits 10, 14
  for (unsigned int i = 0; i < length; i++) {
    // initialize cypher byte
    bytes[i] = 0;
    // compute cypher byte by looping through bits
    for (unsigned int j = 0; j < 8; j++) {
      // compute bit
      bit = (lfsr >> 14);
      // update cypher byte with this bit
      bytes[i] = (bytes[i] << 1) | bit;
      // update the shift register for the next bit
      n = ((lfsr >> 14) ^ (lfsr >> 10)) & 1;
      lfsr = ((lfsr << 1) | n) & 0x7FFF;
    } // END for (j)
  } // END for (i)

}; // END Cypher::Cypher()

void Cypher::Decode(unsigned char *data, unsigned int start) {
  // Uses the cypher bytes to decode data bytes beginning from start.
  //
  // Args:
  //     data (unsigned char *): pointer to data bytes for decoding
  //     start (unsigned int): starting point for the decode

  for (unsigned int i = 0; i < length; i++)
    data[i + start] = data[i + start] ^ bytes[i];

}; // END Cypher::Decode()

void Cypher::Decode32(unsigned char *data, unsigned int start) {
  // Uses the cypher bytes to decode data bytes beginning from start
  // in steps of 32 bits.
  //
  // Args:
  //     data (unsigned char *): pointer to data bytes for decoding
  //     start (unsigned int): starting point for the decode

  unsigned int *data32 = (unsigned int *)(data + start);
  unsigned int *bytes32 = (unsigned int *)bytes;
  for (unsigned int i = 0; i < length32; i++)
    data32[i] = data32[i] ^ bytes32[i];

}; // END Cypher::Decode32()

void Cypher::Decode64(unsigned char *data, unsigned int start) {
  // Uses the cypher bytes to decode data bytes beginning from start
  // in steps of 64 bits.
  //
  // Args:
  //     data (unsigned char *): pointer to data bytes for decoding
  //     start (unsigned int): starting point for the decode

  unsigned long int *data64 = (unsigned long int *)(data + start);
  unsigned long int *bytes64 = (unsigned long int *)bytes;
  for (unsigned int i = 0; i < length64; i++)
    data64[i] = data64[i] ^ bytes64[i];

}; // END Cypher::Decode64()

#endif // DVDCC_CYPHER_H_
