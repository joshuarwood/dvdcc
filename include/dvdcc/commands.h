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

u_int32_t CACHE_SIZE = 80 * 2064;
u_int32_t HITACHI_MEM_BASE = 0x80000000;

int execute_command(int fd, unsigned char *cmd, unsigned char *buffer,
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
     *     (int): the command status where -1 indicates an error
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

    verbose = true;
    if (verbose) {
        printf("Executing MMC command: ");
        for (int i=0; i<6; i++) printf(" %02x%02x", cgc.cmd[2*i], cgc.cmd[2*i+1]);
        printf("\n");
    }

    int status = ioctl(fd, CDROM_SEND_PACKET, &cgc);

    if (verbose)
        printf("Sense data: %02X/%02X/%02X (status %d)\n",
               cgc.sense->sense_key, cgc.sense->asc, cgc.sense->ascq, status);

    return status;    
};

int read_raw_bytes(int fd, int offset, int nbyte, int timeout, bool verbose) {

    u_int32_t address = HITACHI_MEM_BASE + offset;
    unsigned char buffer[65536];

    if ((nbyte <= 0) || (nbyte > 65535)) {
        printf("read_raw_bytes(): invalid nbyte (valid: 1 - 65535)\n");
        exit(0);
    }

    // Note: bytes 1-3 = HIT which is likely short for HITACHI
    unsigned char cmd[12] = {
        0xE7,                         //  0. vendor specific command (discovered by DaveX)
        0x48,                         //  1. H
        0x49,                         //  2. I
        0x54,                         //  3. T
        0x01,                         //  4. read MCU memory sub-command
        0,                            //  5. empty
        (unsigned char)((address & 0xFF000000) >> 24), //  6. address MSB
        (unsigned char)((address & 0x00FF0000) >> 16), //  7. address continued
        (unsigned char)((address & 0x0000FF00) >> 8),  //  8. address continued
        (unsigned char)((address & 0x000000FF)),       //  9. address LSB
        (unsigned char)((nbyte & 0xFF00) >> 8),        // 10. nbyte MSB
        (unsigned char)((nbyte & 0x00FF))              // 11. nbyte LSB
    };

    int status = execute_command(fd, cmd, buffer, nbyte, timeout, verbose);

    return status;
};

int drive_info(int fd, char *model_str, int timeout, bool verbose) {

    const int buflen = 36;
    unsigned char cmd[12], buffer[buflen];

    memset(cmd, 0, 12);
    memset(buffer, 0, buflen);

    cmd[0] = SPC_INQUIRY;
    cmd[4] = buflen;

    int status = execute_command(fd, cmd, buffer, buflen, timeout, verbose);

    char *vendor = strndup((char *)&buffer[8], 8);
    char *prod_id = strndup((char *)&buffer[16], 16);
    char *prod_rev = strndup((char *)&buffer[32], 4);

    sprintf(model_str, "%s/%s/%s", vendor, prod_id, prod_rev);

    return status;
};

int drive_state(int fd, bool state, int timeout, bool verbose) {

    const int buflen = 8;
    unsigned char cmd[12];
    unsigned char buffer[buflen];

    memset(cmd, 0, 12);
    memset(buffer, 0, buflen);

    cmd[0] = 0x1B;
    cmd[4] = (unsigned char)state;

    int status = execute_command(fd, cmd, buffer, buflen, 1, true);

    return status;
};

#endif // DVDCC_COMMANDS_H_
