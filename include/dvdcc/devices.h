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
#include <dvdcc/commands.h>

unsigned int SECTOR_SIZE = 2048;
unsigned int RAW_SECTOR_SIZE = 2064;
unsigned int SECTORS_PER_BLOCK = 16;
unsigned int SECTORS_PER_CACHE = 5 * SECTORS_PER_BLOCK;

/* Dvd class for interfacing with a DVD drive. */
class Dvd {

 public:
   Dvd(const char *path, int timeout, bool verbose);
   ~Dvd() { close(fd); };

   int Start(bool verbose);                                                 // start spinning the disc
   int Stop(bool verbose);                                                  // stop spinning the disc
   int ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose); // read 5 blocks of raw sectors

   int fd;         // file descriptor
   int timeout;    // command timeout in seconds
   char model[36]; // drive model string with vendor/prod_id/prod_rev

}; // END class Dvd()

Dvd::Dvd(const char *path, int timeout = 1, bool verbose = false) : timeout(timeout) {
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
  int status = DriveInfo(fd, model, timeout, verbose);
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

  return DriveState(fd, true, timeout, verbose);

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

  return DriveState(fd, true, timeout, verbose);

}; // END Dvd::Stop()

int Dvd::ReadRawSectorCache(int sector, unsigned char *buffer, bool verbose = false) {
  /* Read all raw sectors from the 80 sector drive cache.
   *
   * Args:
   *     sector (int): starting sector
   *     buffer (unsigned char *): pointer to the buffer where bytes
   *                               returned by the command are placed
   *     verbose (bool): when true print command details (default: false)
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  int status;
  const int buflen = RAW_SECTOR_SIZE * SECTORS_PER_CACHE;

  if (verbose)
    printf("dvdcc:devices:Dvd:ReadRawSectorCache() Reading from sector %d.\n", sector);

  // perform a streaming read with NULL buffer to fill cache with 5 blocks
  // starting from sector. Note: read the first sector to fill the full cache.
  status = DriveReadSectors(fd, buffer, sector, 1, true, timeout, verbose);

  // clear the buffer contents
  memset(buffer, 0, buflen);

  // read the cache in 3 steps to work around the 65535 byte cache read limit
  status = DriveReadRawBytes(fd, buffer +         0,         0, 65535, timeout, verbose);
  status = DriveReadRawBytes(fd, buffer +     65536,     65536, 65535, timeout, verbose);
  status = DriveReadRawBytes(fd, buffer + 2 * 65536, 2 * 65536, 34050, timeout, verbose);

  printf("starting with sector %d\n", sector);
  for (int i=0; i<80; i++) {
    for (int j=0; j<10; j++)
      printf(" %02x", buffer[i * RAW_SECTOR_SIZE + j]);
    printf("\n");
  }

  return status;

}; // END Dvd::ReadRawSectorCache()

#endif // DVDCC_DEVICES_H_
