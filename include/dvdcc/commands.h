//
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

#include "constants.h"
#include "permissions.h"

namespace commands {

int Execute(int fd, unsigned char *cmd, unsigned char *buffer,
            int buflen, int timeout, bool verbose, request_sense *scsi_sense) {
  // Sends a command to the DVD drive using Linux API
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     cmd (unsigned char *): pointer to the 12 command bytes
  //     buffer (unsigned char *): pointer to the buffer where bytes
  //                               returned by the command are placed
  //     buflen (int): length of the buffer
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

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
    printf("dvdcc:commands:Execute() Executing MMC command");
    for (int i=0; i<6; i++) printf(" %02x%02x", cgc.cmd[2*i], cgc.cmd[2*i+1]);
      printf("\n");
  }

  int status = ioctl(fd, CDROM_SEND_PACKET, &cgc);

  if (verbose)
    printf("dvdcc:commands:Execute() Sense data %02X/%02X/%02X (status %d)\n",
           cgc.sense->sense_key, cgc.sense->asc, cgc.sense->ascq, status);

  if (scsi_sense)
    memcpy(scsi_sense, &sense, sizeof(sense));

  return status;

}; // END commands::Execute()

int ReadSectors(int fd, unsigned char *buffer, int sector, int sectors,
                bool streaming, int timeout, bool verbose, request_sense *scsi_sense) {
  // Read 2048 byte data sectors from the drive. These do not include the first
  // 12 bytes (ID, IED, CPR_MAI) or last 4 bytes (EDC) found in raw sectors.
  //
  // Args:
  //     fd (int): file descriptor
  //     buffer (unsigned char *): buffer for returning raw bytes
  //     sector (int): starting sector
  //     sectors (int): number of sectors to read starting from sector
  //     streaming (bool): use cache streaming mode when true.
  //                       otherwise force direct access
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to True to print more info
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[ 0] = constants::MMC_READ_12;                         // read command
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

  return Execute(fd, cmd, buffer, sectors * constants::SECTOR_SIZE, timeout, verbose, scsi_sense);

}; // END commands::ReadSectors()

int ReadRawBytes(int fd, unsigned char *buffer, int offset, int nbyte,
                 int timeout, bool verbose, request_sense *scsi_sense) {
  // Reads raw bytes from the drive cache. This cache consists of 2064 byte
  // raw sectors with ID, IED, CPR_MAI, USER DATA, and EDC fields.
  //
  // Note you *must* do the following before using this command:
  //   1. Execute a read_sectors() command with streaming = True to fill the cache.
  //   2. Ensure you're running the command with root privileges.
  //      The command will not work with regular user privileges.
  //
  // Args:
  //     fd (int): file descriptor
  //     buffer (unsigned char *): buffer for returning raw bytes
  //     offset (int): starting memory offset within cache
  //     nbyte (int): number of memory bytes to read starting from offset
  //     timeout (int, optional): command timeout in seconds
  //     verbose (bool, optional): set to True to print more info
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  u_int32_t address = constants::HITACHI_MEM_BASE + offset;
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

  // vendor command requires root privileges
  permissions::EnableRootPrivileges();

  int status = Execute(fd, cmd, buffer, nbyte, timeout, verbose, scsi_sense);

  // restore original user privileges
  permissions::DisableRootPrivileges();

  return status;

}; // END commands::ReadRawBytes()

int ClearCache(int fd, int sector, int timeout, bool verbose, request_sense *scsi_sense) {
  // Clears the drive cache by forcing a sector read with zero length.
  //
  // Args:
  //     fd (int): file descriptor
  //     sector (int): starting sector
  //     timeout (int, optional): command timeout in seconds
  //     verbose (bool, optional): set to True to print more info
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[0] = constants::MMC_READ_12;                       // read command
  cmd[1] = 0x08;                                         // force unit access bit
  cmd[2] = (unsigned char)((sector & 0xFF000000) >> 24); // sector MSB
  cmd[3] = (unsigned char)((sector & 0x00FF0000) >> 16); // sector continued
  cmd[4] = (unsigned char)((sector & 0x0000FF00) >> 8);  // sector continued
  cmd[5] = (unsigned char) (sector & 0x000000FF);        // sector LSB

  return Execute(fd, cmd, NULL, 0, timeout, verbose, scsi_sense);

}; // END commands::ClearCache()

int Info(int fd, char *model_str, int timeout, bool verbose, request_sense *scsi_sense) {
  // Retrieves the drive model string as vendor/prod_id/prod_rev.
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     model_str (char *): the drive model string to return
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  const int buflen = 36;
  unsigned char cmd[12], buffer[buflen];

  memset(cmd, 0, 12);
  memset(buffer, 0, buflen);

  cmd[0] = constants::SPC_INQUIRY;
  cmd[4] = buflen;

  int status = Execute(fd, cmd, buffer, buflen, timeout, verbose, scsi_sense);

  char *vendor = strndup((char *)&buffer[8], 8);
  char *prod_id = strndup((char *)&buffer[16], 16);
  char *prod_rev = strndup((char *)&buffer[32], 4);

  sprintf(model_str, "%s/%s/%s", vendor, prod_id, prod_rev);

  return status;

}; // END commands::Info()

int StartStop(int fd, bool start, bool loej, unsigned char power, int timeout, bool verbose, request_sense *scsi_sense) {
  // Performs disc start/stop as well as load/eject.
  //
  // LoEj   Start   Operation
  //  0       0     Stop the disc
  //  0       1     Start the disc and make ready for access
  //  1       0     Eject the disc if permitted
  //  1       1     Load the disc
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     start (bool): start the disc spinning when true, stop when false
  //     loej (bool): eject the disc if permitted when true and start is false,
  //                  load the disc if true and start is true
  //     power (unsigned char): power condition (0 = no change, 2 = idle, 3 = standby, 5 = sleep)
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  const int buflen = 8;
  unsigned char cmd[12];
  unsigned char buffer[buflen];

  memset(cmd, 0, 12);
  memset(buffer, 0, buflen);

  cmd[0] = constants::SBC_START_STOP;
  cmd[4] = (unsigned char)start + (((unsigned char)loej) << 1) + (power << 4);

  return Execute(fd, cmd, buffer, buflen, timeout, verbose, scsi_sense);

}; // END commands::StartStop()

int PreventRemoval(int fd, bool prevent, int timeout, bool verbose, request_sense *scsi_sense) {
  // Set the prevent disc removal state.
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     prevent (bool): prevent disc removal when true, allow when false
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  const int buflen = 8;
  unsigned char cmd[12];
  unsigned char buffer[buflen];

  memset(cmd, 0, 12);
  memset(buffer, 0, buflen);

  cmd[0] = 0x1E;
  cmd[4] = (unsigned char)prevent;

  return Execute(fd, cmd, buffer, buflen, timeout, verbose, scsi_sense);

}; // END commands::PreventRemoval()

int GetEventStatus(int fd, unsigned char *buffer, constants::EventType event_type, bool poll,
                   unsigned int allocation, int timeout, bool verbose, request_sense *scsi_sense) {
  // Get an event status notification.
  //
  // Type  Description         Constant
  // 0x02  Operational Change  constants::EventType::kOperationalChange
  // 0x04  Power Management    constants::EventType::kPowerManagement
  // 0x08  External Request    constants::EventType::kExternalRequest
  // 0x10  Media               constants::EventType::kMedia
  // 0x40  Device Busy         constants::EventType::kDeviceBusy
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     buffer (unsigned char *): pointer to the buffer where bytes
  //                               returned by the command are placed
  //     event_type (unsigned char): event type (see Description above)
  //     poll (bool): use polling method when true
  //     allocation (unsigned int): byte allocation (>4 returns notification packet)
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[0] = 0x4A;                                        // get event status notification command
  cmd[1] = (unsigned char)poll;                         // use polling when true
  cmd[4] = (unsigned char)event_type;                   // event type
  cmd[7] = (unsigned char)((allocation & 0xFF00) >> 8); // allocation MSB
  cmd[8] = (unsigned char) (allocation & 0x00FF);       // allocation LSB

  return Execute(fd, cmd, buffer, allocation, timeout, verbose, scsi_sense);

}; // END commands::GetEventStatus()

int TestUnitReady(int fd, int timeout, bool verbose, request_sense *scsi_sense) {
  // Test if the drive is ready to receive commands.
  //
  // Args:
  //     fd (int): the file descriptor of the drive
  //     timeout (int): timeout duration in integer seconds
  //     verbose (bool): set to true to print more details to stdout
  //     scsi_sense (request_sense *): pointer to SCSI sense keys
  //
  // Returns:
  //     (int): command status (-1 means fail)

  unsigned char cmd[12];
  unsigned char buffer[8];

  memset(cmd, 0, 12); // empty command corresponds to test unit ready 0x00

  return Execute(fd, cmd, buffer, 8, timeout, verbose, scsi_sense);

}; // END commands::TestUnitReady()

} // namespace commands

#endif // DVDCC_COMMANDS_H_
