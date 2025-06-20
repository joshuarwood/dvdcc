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

/* Dvd class for interfacing with a DVD drive. */
class Dvd {

 public:
   Dvd(const char *path, int timeout, bool verbose);
   ~Dvd() { close(fd); };

   int Start(bool verbose); // start spinning the disc
   int Stop(bool verbose);  // stop spinning the disc

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
    printf("dvdcc:devices:Dvd() Starting the drive.\n");

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
    printf("dvdcc:devices:Dvd() Stopping the drive.\n");

  return DriveState(fd, true, timeout, verbose);

}; // END Dvd::Stop()

#endif // DVDCC_DEVICES_H_
