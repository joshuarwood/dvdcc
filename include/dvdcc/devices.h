// Copyright (C) 2025     Josh Wood
//
// This portion is based on the friidump project written by:
//              Arep
//              https://github.com/bradenmcd/friidump
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

#ifndef DVDCC_DEVICES_H_
#define DVDCC_DEVICES_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/cdrom.h>

#include <string>
#include <map>

#include "dvdcc/cypher.h"
#include "dvdcc/ecma_267.h"
#include "dvdcc/progress.h"
#include "dvdcc/commands.h"
#include "dvdcc/constants.h"

// Class for interfacing with a DVD drive.
class Dvd {

 public:
  Dvd(const char *path, int timeout, bool verbose);
  ~Dvd() {
    for (int i = 0; i < cypher_number; i++) delete cyphers[i];
    close(fd);
  };

  int Start(bool verbose);                                                 // start spinning the disc
  int Stop(bool verbose);                                                  // stop spinning the disc
  int Load(bool verbose);                                                  // load the disc
  int Eject(bool verbose);                                                 // eject the disc
  int PollReady(bool verbose);                                             // poll the drive ready state
  int PollPowerState(bool verbose);                                        // return the drive power state
  int ClearSectorCache(int sector, bool verbose);                          // clear cached blocks of raw sectors
  int ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose); // read 5 blocks of raw sectors
  int FindKeys(unsigned int blocks, bool verbose);                         // find the keys for decoding sectors
  int FindDiscType(bool verbose);                                          // find the disc type (standard, gamecube, wii, etc)
  int DisplayMetaData(bool verbose);                                       // display disc metadata from the first sector

  unsigned int RawSectorId(unsigned char *raw_sector);                     // return sector id number
  unsigned int RawSectorEdc(unsigned char *raw_sector);                    // return sector error detection code
  unsigned int CypherIndex(unsigned int block);                            // return cypher index for a block

  int fd;                           // file descriptor
  int timeout;                      // command timeout in seconds
  char model[36];                   // drive model string with vendor/prod_id/prod_rev
  unsigned int cypher_number;       // number of cypher keys
  unsigned int sector_number;       // number of disc sectors
  std::string disc_type;            // disc type

  Cypher *cyphers[20];              // cyphers for decoding raw sectors

}; // END class Dvd()

Dvd::Dvd(const char *path, int timeout = 1, bool verbose = false)
    : timeout(timeout), cypher_number(0), sector_number(0), disc_type("UNKOWN"), cyphers{} {
  // Constructor that opens a connection to the DVD drive.
  //
  // Args:
  //     path (const char *): path to the drive, typically /dev/sr0
  //     timeout (int): timeout duration in integer seconds (default: 1)
  //     verbose (bool): when true print details from drive_info() command (default: false) 

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
  // Start spinning the disc.
  //
  // Args:
  //     verbose (bool): when true print details from drive_state() command (default: false)
  //
  // Returns:
  //     (int): command status (-1 means fail)

  if (verbose)
    printf("dvdcc:devices:Dvd:Start() Starting the drive.\n");

  return commands::StartStop(fd, true, false, 0, timeout, verbose, NULL);

}; // END Dvd::Start()

int Dvd::Stop(bool verbose = false) {
  // Stop spinning the disc.
  //
  // Args:
  //     verbose (bool): when true print details from drive_state() command (default: false)
  //
  // Returns:
  //     (int): command status (-1 means fail)

  if (verbose)
    printf("dvdcc:devices:Dvd:Stop() Stopping the drive.\n");

  return commands::StartStop(fd, false, false, 0, timeout, verbose, NULL);

}; // END Dvd::Stop()

int Dvd::Load(bool verbose = false) {
  // Load the disc.
  //
  // Args:
  //     verbose (bool): when true print details from StartStop() command (default: false)
  //
  // Returns:
  //     (int): command status (-1 means fail)

  if (verbose)
    printf("dvdcc:devices:Dvd:Load() Loading the drive.\n");

  return commands::StartStop(fd, true, true, 0, timeout, verbose, NULL);

}; // END Dvd::Load()

int Dvd::Eject(bool verbose = false) {
  // Eject the disc.
  //
  // Args:
  //     verbose (bool): when true print details from commands (default: false)
  //
  // Returns:
  //     (int): command status (-1 means fail)

  if (verbose)
    printf("dvdcc:devices:Dvd:Eject() Ejecting the disc.\n");

  // enable removal
  commands::PreventRemoval(fd, false, timeout, verbose, NULL);

  // eject
  return commands::StartStop(fd, false, true, 0, timeout, verbose, NULL);

}; // END Dvd::Eject()

int Dvd::ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose = false) {
  // Read all raw sectors from the 80 sector cache.
  //
  // Args:
  //     sector (int): starting sector relative to the first disc sector
  //     buffer (unsigned char *): pointer to the buffer where bytes
  //                               returned by the command are placed
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): command status (-1 means fail)

  int status;
  const int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;

  if (verbose)
    printf("dvdcc:devices:Dvd:ReadRawSectorCache() Reading cache with sector %d.\n", sector);

  // perform a streaming read to fill the cache with 5 blocks / 80 sectors
  // starting from sector. Note: reading first sector to fills the full cache.
  if (commands::ReadSectors(fd, buffer, start_sector, 1, true, timeout, verbose, NULL) != 0)
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
  // Return the sector id number from the first 4 bytes of raw sector data.
  //
  // Args:
  //     raw_sector (unsigned char *): raw sector data
  //
  // Returns:
  //     (unsigned int): sector id number

  return (raw_sector[0] << 24) + (raw_sector[1] << 16) + (raw_sector[2] << 8) + raw_sector[3];

}; // END Dvd::RawSectorId()

unsigned int Dvd::RawSectorEdc(unsigned char *raw_sector) {
  // Return the error detection code from the last 4 bytes of raw sector data.
  //
  // Args:
  //     raw_sector (unsigned char *): raw sector data
  //
  // Returns:
  //     (unsigned int): error detection code

  unsigned char *edc_bytes = raw_sector + constants::RAW_SECTOR_SIZE - 4;

  return (edc_bytes[0] << 24) + (edc_bytes[1] << 16) + (edc_bytes[2] << 8) + edc_bytes[3];

}; // END Dvd::RawSectorEdc()

unsigned int Dvd::CypherIndex(unsigned int block) {
  // Return the cypher array index for a sector block.
  //
  // Blocks >= 1 use a repeating sequence of cypher values
  // from 1 to cypher_number. The first block beyond cypher_number
  // loops back to 1 to restart the sequence.
  //
  // Block 0 uses a unique cypher to decode disc information.
  // The cypher index for this block is handled as a separate
  // return value because it is not part of the repeating sequence.
  //
  // Args:
  //     block (unsigned int): block number
  //
  // Returns:
  //     (unsigned int): cypher index

  if (block)
    return (block - 1) % (cypher_number - 1) + 1;
  return 0;

}; // END Dvd::CypherIndex()

int Dvd::FindKeys(unsigned int blocks = 20, bool verbose = false) {
  // Find the cypher keys needed to decode raw sector data.
  // Should only need 20 blocks since there is usually one
  // key used to decode the first block followed by a
  // repeating sequence of 16 keys for the remaining blocks.
  //
  // Args:
  //     blocks (int): number of blocks to check starting from the first sector (default: 20)
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): command status (0 = success, -1 = fail)

  Cypher *cypher;

  unsigned int raw_sector_id, raw_edc, tmp_edc;
  const unsigned int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;
  unsigned char buffer[buflen];
  unsigned char *raw_sector, tmp[constants::RAW_SECTOR_SIZE];

  bool found_all_cyphers = false; // set to true once we find all cyphers

  printf("Finding DVD keys...\n\n");

  // loop through blocks of sectors to find the cypher for each block
  for (unsigned int block = 0; block < blocks; block++) {

    // fill the buffer from cache whenever we're outside last cache read
    if (block % constants::BLOCKS_PER_CACHE == 0)
      ReadRawSectorCache(block * constants::SECTORS_PER_BLOCK, buffer, verbose);

    // assign cypher if all cyphers are found, otherwise set to NULL to find a new cypher
    cypher = found_all_cyphers ? cyphers[CypherIndex(block)] : NULL;

    for (unsigned int sub_sector = 0; sub_sector < constants::SECTORS_PER_BLOCK; sub_sector++) {

      // get the raw sector for this sub sector of the block from the buffer
      raw_sector = buffer + (sub_sector + block % constants::BLOCKS_PER_CACHE * constants::SECTORS_PER_BLOCK) * constants::RAW_SECTOR_SIZE;

      // gather error detection code for the raw sector
      raw_edc = RawSectorEdc(raw_sector);

      if (cypher == NULL) {

        // loop through seeds to find the correct cypher
        for (unsigned int seed = 0; seed < 0x7FFF; seed++) {
	  // create a temporary copy of raw_sector
          memcpy(tmp, raw_sector, constants::RAW_SECTOR_SIZE);
          // try decoding
          cypher = new Cypher(seed, constants::SECTOR_SIZE);
	  cypher->Decode64(tmp, 12);
	  // verify edc
	  if (raw_edc == ecma_267::calculate(tmp, constants::RAW_SECTOR_SIZE - 4)) {
            // repeated seed 1 means we found all cyphers
            if (cyphers[1] && cyphers[1]->seed == cypher->seed)
	      found_all_cyphers = true;
	    else
              printf(" * Block %02d found key 0x%04x\n", block, cypher->seed);
	    // decode the raw_sector now that we have the correct cypher
	    // Note: this could be removed, but I left it here in case
	    // we decide to consolidate key finding with a full disc read.
            cypher->Decode64(raw_sector, 12);
            break;
	  } else {
	    delete cypher;
	    cypher = NULL;
	  } // END if/else (raw_edc == ...)
        } // END for (seed)

        // throw and error if we couldn't find the cypher
        if (cypher == NULL) {
          printf("dvdcc:devices:Dvd::FindKeys() Could not identify cypher %02d\n", cypher_number);
	  return -1;
        } // END if (cypher == NULL)

      } else {

	// verify edc for remaining sectors in the block
        cypher->Decode64(raw_sector, 12);
        if (raw_edc != ecma_267::calculate(raw_sector, constants::RAW_SECTOR_SIZE - 4)) {
          printf("dvdcc:devices:Dvd::FindKeys() Failed to decode sector with seed %04x\n", cypher->seed);
	  return -1;
        } // END if (raw_edc != ...)

      } // END if/else (cypher == NULL)

    } // END for (sub_sector)

    if (found_all_cyphers == false) {
      // copy the cypher into the cyphers array
      // and delete before evaluating the next block
      cyphers[cypher_number++] = new Cypher(cypher->seed, cypher->length);
      delete cypher;
    } // END if (found_all_cyphers == false)

  } // END for (block)

  printf("\nDone.\n\n");

  return 0;

}; // END Dvd::FindKeys()

int Dvd::FindDiscType(bool verbose = false) {
  // Find the disc type and sector number for a disc.
  //
  // Args:
  //     verbose (bool): when true print command details (default: false).
  //
  // Returns:
  //     (int): command status (0 = success, -1 = fail)

  unsigned char buffer[constants::SECTOR_SIZE];
  struct request_sense sense;

  printf("Finding Disc Type...\n\n");

  // loop through known sector numbers / disc types
  std::map<unsigned int, std::string>::iterator it;
  for (it = constants::sector_numbers.begin(); it != constants::sector_numbers.end(); it++) {

    // parse this iteration
    sector_number = it->first;
    disc_type = it->second;

    // test sense keys just beyond the sector number to verify type
    memset(&sense, 0, sizeof(sense));
    commands::ReadSectors(fd, buffer, sector_number + 100, 1, false, timeout, verbose, &sense);

    if (sense.sense_key == 0x05 && sense.asc == 0x21) {
      printf("Found %s with %d sectors.\n\n", disc_type.c_str(), sector_number);
      return 0;
    }

  } // END for (it)

  return -1;

}; // END Dvd::FindDiscType()

int Dvd::DisplayMetaData(bool verbose = false) {
  // Display disc metadata on-screen
  //
  // Args:
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): command status (0 = success, -1 = fail)

  double gb = 1024 * 1024 * 1024;

  double iso_size_gb = double(sector_number) * constants::SECTOR_SIZE / gb;
  double raw_size_gb = double(sector_number) * constants::RAW_SECTOR_SIZE / gb;

  printf("Disc information:\n");
  printf("--------------------\n");
  printf("Disc type..........: %s\n", disc_type.c_str());
  printf("Disc size..........: %lu sectors (%.2f GB iso, %.2f GB raw)\n",
         sector_number, iso_size_gb, raw_size_gb);

  // additional fields for Gamecube and WII discs
  if (disc_type == "GAMECUBE" || disc_type == "WII_SINGLE_LAYER" || disc_type == "WII_DUAL_LAYER") {

    // read cache block with first sector
    unsigned char buffer[constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE];
    int status = ReadRawSectorCache(0, buffer, verbose);

    // exit early when there is an error
    if (status != 0) return status;

    // decode the first sector
    cyphers[0]->Decode64(buffer, 12);

    // point to the start of usable data following the 6 sector ID/IED bytes
    unsigned char *data = buffer + 6;

    char *system_id    = strndup((char *)&data[0], 1);
    char *game_id      = strndup((char *)&data[1], 2);
    char *region_id    = strndup((char *)&data[3], 1);
    char *publisher_id = strndup((char *)&data[4], 2);

    // full system name
    std::string system = "UNKNOWN";
    if (auto search = constants::systems.find(system_id); search != constants::systems.end())
      system = search->second;

    // full region name
    std::string region = "UNKNOWN";
    if (auto search = constants::regions.find(region_id); search != constants::regions.end())
      region = search->second;

    // full publisher name
    std::string publisher = "UNKNOWN";
    if (auto search = constants::publishers.find(publisher_id); search != constants::publishers.end())
      publisher = search->second;

    // title without additional whitespace
    char *title        = strndup((char *)&data[0x20], 64);
    for (int i = 63; i >= 0 && title[i] == ' '; i--)
      title[i] = '\0';

    printf("System ID..........: %s (%s)\n", system_id, system.c_str());
    printf("Game ID............: %s\n", game_id);
    printf("Region.............: %s (%s)\n", region_id, region.c_str());
    printf("Publisher..........: %s (%s)\n", publisher_id, publisher.c_str());
    printf("Version............: 1.%02u\n", data[7]);
    printf("Game title.........: %s\n", title);

    // check for additional update information found in sector 160 of Wii discs
    status = ReadRawSectorCache(160, buffer, verbose);
    // decode the sector
    cyphers[CypherIndex(160 / constants::SECTORS_PER_BLOCK)]->Decode64(buffer, 12);
    // point to the start of usable data following the 6 sector ID/IED bytes
    data = buffer + 6;
    // compute the update key value
    unsigned int update_key = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7];
    // check the update key value
    bool has_update = (update_key != 0xA5BED6AE) && (disc_type == "WII_SINGLE_LAYER" || disc_type == "WII_DUAL_LAYER");

    printf("Contains update....: %s (0x%08x)\n\n", has_update ? "Yes" : "No", update_key);

    return status;

  } // END if (disc_type == "GAMECUBE" ...)

  printf("\n");

  return 0;

}; // END Dvd::DisplayMetaData()

int Dvd::ClearSectorCache(int sector, bool verbose = false) {
  // Clear the current sector cache by seeking to the starting sector of
  // the farthest full cache block. This will either be sector 0 or one less
  // than the total number of cache blocks.
  //
  // Args:
  //     sector (int): current sector number
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): command status (0 = success, -1 = fail)

  unsigned int cache_block_number = sector_number / constants::SECTORS_PER_CACHE;
  unsigned int last_cache_sector = (cache_block_number - 1) * constants::SECTORS_PER_CACHE;
  unsigned int farthest_sector = sector < last_cache_sector - sector ? last_cache_sector : 0;

  unsigned char buffer[constants::SECTOR_SIZE];

  return commands::ReadSectors(fd, buffer, farthest_sector, constants::SECTORS_PER_CACHE, true, timeout, verbose, NULL);

}; // END Dvd::ClearSectorCache()

int Dvd::PollPowerState(bool verbose = false) {
  // Get the drive power state.
  //
  // Args:
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): power state (1 = Active, 2 = Idle, 3 = Standy, 4 = Sleep)

  const int buflen = 16;
  unsigned char buffer[buflen];

  bool poll = true;

  int status = commands::GetEventStatus(fd, buffer, constants::EventType::kPowerManagement, poll, buflen, timeout, verbose, NULL);

  if (status < 0)
    return status;

  return (int)buffer[5];

}; // END Dvd::PollPowerState()

int Dvd::PollReady(bool verbose = false) {
  // Get the test unit ready status.
  //
  // Args:
  //     verbose (bool): when true print command details (default: false)
  //
  // Returns:
  //     (int): status (0 = ready to receive commands, -1 = not ready)

  return commands::TestUnitReady(fd, timeout, verbose, NULL);

}; // END Dvd::PollReady()

#endif // DVDCC_DEVICES_H_
