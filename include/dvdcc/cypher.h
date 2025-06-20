/* Copyright (C) 2025     Josh Wood
 *
 * This portion is very closely based on the unscrambler project
 * written by:
 *              Victor Muñoz (xt5@ingenieria-inversa.cl)
 *              https://github.com/saramibreak/unscrambler
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef DVDCC_CYPHER_H_
#define DVDCC_CYPHER_H_

#include <stdlib.h>

/* Cypher class for creating the cypher key used to decode raw DVD data. */
class Cypher {

 public:
   Cypher(unsigned int seed, unsigned int length);
   ~Cypher() { free(bytes); };

   unsigned int seed;
   unsigned int length;
   unsigned char *bytes;

}; // END class Cypher()

Cypher::Cypher(unsigned int seed, unsigned int length) : seed(seed), length(length) {
  /* Constructor that generates a cypher used to decode raw DVD data.
   *
   * Note:
   *     Cypher generation is implemented as a 15 bit
   *     Linear Feedback Shift Register (LFSR) with
   *     bits 10 and 14 as taps. See:
   *
   * [1] https://en.wikipedia.org/wiki/Linear-feedback_shift_register
   * [2] https://hitmen.c02.at/files/docs/gc/Ingenieria-Inversa-Understanding_WII_Gamecube_Optical_Disks.html
   *
   * Args:
   *     seed (unsigned int): seed value for the cypher construction
   *     length (unsigned int): desired length of the cypher in bytes
   *
   * Returns:
   *     (unsigned char *)
   */
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

#endif // DVDCC_CYPHER_H_
