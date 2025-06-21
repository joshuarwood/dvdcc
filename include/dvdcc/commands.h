/*
 * Copyright (C) 2025     Josh Wood
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
#ifndef DVDCC_COMMANDS_H_
#define DVDCC_COMMANDS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/cdrom.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

u_int8_t SPC_INQUIRY = 0x12;
u_int8_t MMC_READ_12 = 0xA8;

u_int32_t HITACHI_MEM_BASE = 0x80000000;

int ExecuteCommand(int fd, unsigned char *cmd, unsigned char *buffer,
                    int buflen, int timeout, bool verbose) {
  /* Sends a command to the DVD drive using Linux API
   *
   * Args:
   *     fd (int): the file descriptor of the drive
   *     cmd (unsigned char *): pointer to the 12 command bytes
   *     buffer (unsigned char *): pointer to the buffer where bytes
   *                               returned by the command are placed
   *     buflen (int): length of the buffer
   *     timeout (int): timeout duration in integer seconds
   *     verbose (bool): set to true to print more details to stdout
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  struct cdrom_generic_command cgc;
  struct request_sense sense;

  memset(&cgc, 0, sizeof(cgc));
  memset(&sense, 0, sizeof(sense));
  memcpy(cgc.cmd, cmd, 12);

  cgc.buffer = buffer;
  cgc.buflen = buflen;
  cgc.sense = &sense;
  cgc.data_direction = CGC_DATA_READ;
  cgc.timeout = timeout * 1000;

  if (verbose) {
    printf("dvdcc:commands:ExecuteCommand() Executing MMC command");
    for (int i=0; i<6; i++) printf(" %02x%02x", cgc.cmd[2*i], cgc.cmd[2*i+1]);
      printf("\n");
  }

  int status = ioctl(fd, CDROM_SEND_PACKET, &cgc);

  if (verbose)
    printf("dvdcc:commands:ExecuteCommand() Sense data %02X/%02X/%02X (status %d)\n",
           cgc.sense->sense_key, cgc.sense->asc, cgc.sense->ascq, status);

  return status;

}; // END ExecuteCommand()

int DriveReadSectors(int fd, unsigned char *buffer, int sector, int sectors, bool streaming, int timeout, bool verbose) {
  /* Read 2048 byte data sectors from the drive. These do not include the first
   * 12 bytes (ID, IED, CPR_MAI) or last 4 bytes (EDC) found in raw sectors.
   *
   * Args:
   *     fd (int): file descriptor
   *     buffer (unsigned char *): buffer for returning raw bytes
   *     sector (int): starting sector
   *     sectors (int): number of sectors to read starting from sector
   *     streaming (bool): use cache streaming mode when true.
   *                       otherwise force direct access
   *     timeout (int): timeout duration in integer seconds
   *     verbose (bool): set to True to print more info
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[ 0] = MMC_READ_12;                                    // read command
  cmd[ 1] = streaming ? 0 : 0x08;                           // force unit access bit
  cmd[ 2] = (unsigned char)(( sector & 0xFF000000) >> 24);  // sector MSB
  cmd[ 3] = (unsigned char)(( sector & 0x00FF0000) >> 16);  // sector continued
  cmd[ 4] = (unsigned char)(( sector & 0x0000FF00) >> 8);   // sector continued
  cmd[ 5] = (unsigned char) ( sector & 0x000000FF);         // sector LSB
  cmd[ 6] = (unsigned char)((sectors & 0xFF000000) >> 24);  // sectors MSB
  cmd[ 7] = (unsigned char)((sectors & 0x00FF0000) >> 16);  // sectors continued
  cmd[ 8] = (unsigned char)((sectors & 0x0000FF00) >> 8);   // sectors continued
  cmd[ 9] = (unsigned char) (sectors & 0x000000FF);         // sectors LSB
  cmd[10] = streaming ? 0x80 : 0;                           // streaming bit

  return ExecuteCommand(fd, cmd, buffer, sectors * 2048, timeout, verbose);

}; // END DriveReadSectors()

int DriveReadRawBytes(int fd, unsigned char *buffer, int offset, int nbyte, int timeout, bool verbose) {
  /* Reads raw bytes from the drive cache. This cache consists of 2064 byte
   * raw sectors with ID, IED, CPR_MAI, USER DATA, and EDC fields.
   *
   * Note you *must* do the following before using this command:
   *   1. Execute a read_sectors() command with streaming = True to fill the cache.
   *   2. Ensure you're running the command with root privileges.
   *      The command will not work with regular user privileges.
   *
   * Args:
   *     fd (int): file descriptor
   *     buffer (unsigned char *): buffer for returning raw bytes
   *     offset (int): starting memory offset within cache
   *     nbyte (int): number of memory bytes to read starting from offset
   *     timeout (int, optional): command timeout in seconds
   *     verbose (bool, optional): set to True to print more info
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  u_int32_t address = HITACHI_MEM_BASE + offset;
  unsigned char cmd[12];

  if ((nbyte <= 0) || (nbyte > 65535)) {
    printf("dvdcc:commands:read_raw_bytes() invalid nbyte (valid: 1 - 65535)\n");
    return -1;
  }

  memset(cmd, 0, 12);

  cmd[ 0] = 0xE7;                                          // vendor specific command (discovered by DaveX)
  cmd[ 1] = 0x48;                                          // H
  cmd[ 2] = 0x49;                                          // I
  cmd[ 3] = 0x54;                                          // T
  cmd[ 4] = 0x01;                                          // read MCU memory sub-command
  cmd[ 6] = (unsigned char)((address & 0xFF000000) >> 24); // address MSB
  cmd[ 7] = (unsigned char)((address & 0x00FF0000) >> 16); // address continued
  cmd[ 8] = (unsigned char)((address & 0x0000FF00) >> 8);  // address continued
  cmd[ 9] = (unsigned char) (address & 0x000000FF);        // address LSB
  cmd[10] = (unsigned char)((  nbyte & 0xFF00) >> 8);      // nbyte MSB
  cmd[11] = (unsigned char) (  nbyte & 0x00FF);            // nbyte LSB

  return ExecuteCommand(fd, cmd, buffer, nbyte, timeout, verbose);

}; // END DriveReadRawBytes()

int DriveClearCache(int fd, int sector, int timeout, bool verbose) {
  /* Clears the drive cache by forcing a sector read with zero length.
   *
   * Args:
   *     fd (int): file descriptor
   *     sector (int): starting sector
   *     timeout (int, optional): command timeout in seconds
   *     verbose (bool, optional): set to True to print more info
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[0] = MMC_READ_12;                                  // read command
  cmd[1] = 0x08;                                         // force unit access bit
  cmd[2] = (unsigned char)((sector & 0xFF000000) >> 24); // sector MSB
  cmd[3] = (unsigned char)((sector & 0x00FF0000) >> 16); // sector continued
  cmd[4] = (unsigned char)((sector & 0x0000FF00) >> 8);  // sector continued
  cmd[5] = (unsigned char) (sector & 0x000000FF);        // sector LSB

  return ExecuteCommand(fd, cmd, NULL, 0, timeout, verbose);

}; // END DriveClearCache()

int DriveInfo(int fd, char *model_str, int timeout, bool verbose) {
  /* Retrieves the drive model string as vendor/prod_id/prod_rev.
   *
   * Args:
   *     fd (int): the file descriptor of the drive
   *     model_str (char *): the drive model string to return
   *     timeout (int): timeout duration in integer seconds
   *     verbose (bool): set to true to print more details to stdout
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  const int buflen = 36;
  unsigned char cmd[12], buffer[buflen];

  memset(cmd, 0, 12);
  memset(buffer, 0, buflen);

  cmd[0] = SPC_INQUIRY;
  cmd[4] = buflen;

  int status = ExecuteCommand(fd, cmd, buffer, buflen, timeout, verbose);

  char *vendor = strndup((char *)&buffer[8], 8);
  char *prod_id = strndup((char *)&buffer[16], 16);
  char *prod_rev = strndup((char *)&buffer[32], 4);

  sprintf(model_str, "%s/%s/%s", vendor, prod_id, prod_rev);

  return status;

}; // END DriveInfo()

int DriveState(int fd, bool state, int timeout, bool verbose) {
  /* Toggles the drive state where true = spinning, false = stopped.
   *
   * Args:
   *     fd (int): the file descriptor of the drive
   *     state (bool): drive state (true = spinning, false = stopped)
   *     timeout (int): timeout duration in integer seconds
   *     verbose (bool): set to true to print more details to stdout
   *
   * Returns:
   *     (int): command status (-1 means fail)
   */
  const int buflen = 8;
  unsigned char cmd[12];
  unsigned char buffer[buflen];

  memset(cmd, 0, 12);
  memset(buffer, 0, buflen);

  cmd[0] = 0x1B;
  cmd[4] = (unsigned char)state;

  return ExecuteCommand(fd, cmd, buffer, buflen, timeout, verbose);

}; // END DriveState()

#endif // DVDCC_COMMANDS_H_
