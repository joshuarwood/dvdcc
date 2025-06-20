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


void generate_cypher(unsigned int seed, unsigned char *cypher, int length) {
    /* Generates the 2048 byte cypher used to decode raw DVD data.
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
    unsigned int n, bit;

    // initialize the shift register
    unsigned int lfsr = seed;

    // loop to calculate bytes by shifting lfsr and
    // then updating lfsr using taps at bits 10, 14
    for (int i = 0; i < length; i++) {
	// initialize cypher byte
	cypher[i] = 0;
	// comput cypher byte by looping through bits
        for (int j = 0; j < 8; j++) {
            // compute bit
            bit = (lfsr >> 14);
            // update cyphre byte with this bit
            cypher[i] = (cypher[i] << 1) | bit;
            // update the shift register
            n = ((lfsr >> 14) ^ (lfsr >> 10)) & 1;
            lfsr = ((lfsr << 1) | n) & 0x7FFF;
	}
    }

    return;
};
