/* Copyright (C) 2025     Josh Wood
 *
 * This portion is based on the friidump project written by:
 *              Arep
 *              https://github.com/bradenmcd/friidump
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
#ifndef DVDCC_DEVICES_H_
#define DVDCC_DEVICES_H_

#include <unistd.h>
#include <fcntl.h>

#include "dvdcc/cypher.h"
#include "dvdcc/ecma_267.h"
#include "dvdcc/progress.h"
#include "dvdcc/commands.h"
#include "dvdcc/constants.h"

/* Dvd class for interfacing with a DVD drive. */
class Dvd {

 public:
  Dvd(const char *path, int timeout, bool verbose);
  ~Dvd() {
    for (int i = 0; i < cypher_number; i++)
      delete cyphers[i];
    close(fd);
  };

  int Start(bool verbose);                                                 // start spinning the disc
  int Stop(bool verbose);                                                  // stop spinning the disc
  int ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose); // read 5 blocks of raw sectors
  int FindKeys(unsigned int blocks, bool verbose);                         // find the keys for decoding sectors

  unsigned int RawSectorId(unsigned char *raw_sector);                     // return sector id number
  unsigned int RawSectorEdc(unsigned char *raw_sector);                    // return sector error detection code
  unsigned int CypherIndex(unsigned int sector);                           // return cypher index for a sector

  int fd;                           // file descriptor
  int timeout;                      // command timeout in seconds
  char model[36];                   // drive model string with vendor/prod_id/prod_rev
  unsigned int cypher_number;       // number of cypher keys
  unsigned int first_raw_sector_id; // first raw sector id number of the DVD

  Cypher *cyphers[20];              // cyphers for decoding raw sectors

}; // END class Dvd()

Dvd::Dvd(const char *path, int timeout = 1, bool verbose = false)
    : timeout(timeout), cypher_number(0), first_raw_sector_id(0), cyphers{NULL, NULL} {
  /* Constructor that opens a connection to the DVD drive.
   *
   * Args:
   *     path (const char *): path to the drive, typically /dev/sr0
   *     timeout (int): timeout duration in integer seconds (default: 1)
   *     verbose (bool): when true print details from drive_info() command (default: false) 
   */
  if (verbose)
    printf("dvdcc:devices:Dvd() Opening %s\n", path);

  fd = open(path, O_RDONLY | O_NONBLOCK);

  // read and store the model string
  int status = commands::Info(fd, model, timeout, verbose, NULL);
  if (status != 0) {
    printf("dvdcc:devices:Dvd() Could not determine drive model for %s\n", path);
    printf("dvdcc:devices:Dvd() Exiting...\n");
    exit(0);
  }

}; // END Dvd::Dvd()

int Dvd::Start(bool verbose = false) {
  /* Start spinning the disc.
   *
   * Args:
   *     verbose (bool): when true print details from drive_state() command (default: false)
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  if (verbose)
    printf("dvdcc:devices:Dvd:Start() Starting the drive.\n");

  return commands::Spin(fd, true, timeout, verbose, NULL);

}; // END Dvd::Start()

int Dvd::Stop(bool verbose = false) {
  /* Stop spinning the disc.
   *
   * Args:
   *     verbose (bool): when true print details from drive_state() command (default: false)
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  if (verbose)
    printf("dvdcc:devices:Dvd:Stop() Stopping the drive.\n");

  return commands::Spin(fd, true, timeout, verbose, NULL);

}; // END Dvd::Stop()

int Dvd::ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose = false) {
  /* Read all raw sectors from the 80 sector cache.
   *
   * Args:
   *     sector (int): starting sector relative to the first disc sector
   *     buffer (unsigned char *): pointer to the buffer where bytes
   *                               returned by the command are placed
   *     verbose (bool): when true print command details (default: false)
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  int status;
  const int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;

  if (verbose)
    printf("dvdcc:devices:Dvd:ReadRawSectorCache() Reading from sector %d.\n", sector);

  // perform a streaming read to fill the cache with 5 blocks / 80 sectors
  // starting from sector. Note: reading first sector to fills the full cache.
  if (commands::ReadSectors(fd, buffer, sector, 1, true, timeout, verbose, NULL) != 0)
    return -1;

  // clear the buffer contents
  memset(buffer, 0, buflen);

  // read the cache in steps to work around the 65535 byte cache read limit
  for (int i = 0; i < buflen; i += 65535) {
    int len = i + 65535 <= buflen ? 65535 : buflen - i;
    if (commands::ReadRawBytes(fd, buffer + i, i, len, timeout, verbose, NULL) != 0)
      return -1;
  }

  return 0;

}; // END Dvd::ReadRawSectorCache()

unsigned int Dvd::RawSectorId(unsigned char *raw_sector) {
  /* Return the sector id number from the first 4 bytes of raw sector data.
   *
   * Args:
   *     raw_sector (unsigned char *): raw sector data
   *
   * Returns:
   *     (unsigned int): sector id number
   */
   return (raw_sector[0] << 24) + (raw_sector[1] << 16) + (raw_sector[2] << 8) + raw_sector[3];

}; // END Dvd::RawSectorId()

unsigned int Dvd::RawSectorEdc(unsigned char *raw_sector) {
  /* Return the error detection code from the last 4 bytes of raw sector data.
   *
   * Args:
   *     raw_sector (unsigned char *): raw sector data
   *
   * Returns:
   *     (unsigned int): error detection code
   */
   unsigned char *edc_bytes = raw_sector + constants::RAW_SECTOR_SIZE - 4;

   return (edc_bytes[0] << 24) + (edc_bytes[1] << 16) + (edc_bytes[2] << 8) + edc_bytes[3];

}; // END Dvd::RawSectorEdc()

unsigned int Dvd::CypherIndex(unsigned int raw_sector_id) {
  /* Return the cypher array index for a raw sector id.
   *
   * Args:
   *     raw_sector_id (unsigned int): raw sector id number
   *
   * Returns:
   *     (unsigned int): cypher index
   */
   unsigned int block_id = (raw_sector_id - first_raw_sector_id) / 16;

   // Note: The cypher indices for regular sectors begin at index 1
   // because index 0 is a unique cypher used to decode disc information.
   return block_id % cypher_number + 1;

}; // END Dvd::CypherIndex()

int Dvd::FindKeys(unsigned int blocks = 20, bool verbose = false) {
  /* Find the cypher keys needed to decode raw sector data.
   *
   * Args:
   *     block (int): number of blocks to check starting from the first sector (default: 20)
   *     verbose (bool): when true print command details (default: false)
   *
   * Returns:
   *     (int): command status (0 = success, -1 = fail)
   */
  Cypher *cypher = NULL;

  unsigned int rel_sector_id, raw_sector_id, raw_edc, tmp_edc;
  const unsigned int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;
  unsigned char buffer[buflen];
  unsigned char *raw_sector, tmp[constants::RAW_SECTOR_SIZE];

  bool found_all_cyphers = false; // set to true once we find all cyphers

  printf("\nFinding DVD keys.\n\n");

  // loop through blocks of sectors to find the cypher for each block
  for (unsigned int block = 0; block < blocks; block++) {

    // starting sector id for this block relative to first disc sector
    rel_sector_id = block * constants::SECTORS_PER_BLOCK;

    // fill the buffer from cache whenever we're outside last cache read
    if (block % constants::BLOCKS_PER_CACHE == 0) {
      printf("Reading cache at sector %d\n", rel_sector_id);
      ReadRawSectorCache(rel_sector_id, buffer, verbose);
      raw_sector = NULL;
    }

    // assign cypher if all cyphers are found, otherwise set to NULL to find a new cypher
    cypher = found_all_cyphers ? cyphers[block % cypher_number + 1] : NULL;
    if (cypher)
      printf("reusing seed %04x\n", cypher->seed);

    // add ReadRawCache here when we hit multiples of the cache size block % BLOCKS_PER_CACHE
    for (unsigned int sector = 0; sector < constants::SECTORS_PER_BLOCK; sector++) {

      if (raw_sector) // increment to next sector when prior sector exists
        raw_sector = raw_sector + constants::RAW_SECTOR_SIZE;
      else // otherwise begin at buffer start
        raw_sector = buffer;

      // store the first raw sector id
      if (block == 0 && sector == 0)
        first_raw_sector_id = RawSectorId(raw_sector);

      // gather error detection code for the raw sector
      raw_edc = RawSectorEdc(raw_sector);

      printf("%d %d cypher id %02d\n", RawSectorId(raw_sector), sector, block);
      for (int i=0; i<10; i++)
        printf(" %02x", raw_sector[i]);
      printf("\n");

      if (cypher == NULL) {

        // loop through seeds to find the correct cypher
        for (unsigned int seed = 0; seed < 0x7FFF; seed++) {
	  // create a temporary copy of raw_sector
          memcpy(tmp, raw_sector, constants::RAW_SECTOR_SIZE);
          // try decoding
          cypher = new Cypher(seed, constants::SECTOR_SIZE);
	  cypher->Decode(tmp, 12);
	  // verify edc
	  if (raw_edc == ecma_267::calculate(tmp, constants::RAW_SECTOR_SIZE - 4)) {
            if (cyphers[1] && cyphers[1]->seed == cypher->seed) {
              // repeated seed means we found all cyphers
	      found_all_cyphers = true;
	      // replace this cypher with the first since they match
	      delete cypher;
	      cypher = cyphers[1];
              printf("reusing cypher %04x\n", cypher->seed);
	    } else {
              printf("%02d Found %04x\n", block, cypher->seed);
	    }
	    // decode the raw_sector now that we have the correct cypher
            cypher->Decode(raw_sector, 12);
            break;
	  } else delete cypher;

        } // END for (seed)

        // verify that we successfully found the cypher
        if (cypher == NULL) {
          printf("dvdcc:devices:Dvd::FindKeys() Could not identify cypher %02d\n", cypher_number);
	  return -1;
        } // END if (cypher == NULL)

      } else {

	// verify edc for remaining sectors in the block
        cypher->Decode(raw_sector, 12);
        if (raw_edc != ecma_267::calculate(raw_sector, constants::RAW_SECTOR_SIZE - 4)) {
          printf("dvdcc:devices:Dvd::FindKeys() Failed to decode sector with seed %04x\n", cypher->seed);
	  return -1;
        } // END if (raw_edc != ...)

      } // END if/else (cypher == NULL)

    } // END for (sector)

    if (found_all_cyphers == false) {
      // copy the cypher into the cyphers array
      // and delete before evaluating the next block
      cyphers[cypher_number++] = new Cypher(cypher->seed, cypher->length);
      delete cypher;
    }

  } // END for (block)

  return 0;

}; // END Dvd::FindKeys()

#endif // DVDCC_DEVICES_H_
