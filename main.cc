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

#include "dvdcc/cypher.h"
#include "dvdcc/devices.h"
#include "dvdcc/ecma_267.h"
#include "dvdcc/commands.h"

int main(void) {

    Dvd dvd("/dev/sr0");
    printf("Drive model: %s\n", dvd.model);
    printf("timeout:     %d\n", dvd.timeout);

    return 0;

    Cypher cypher(0x180, 2048);
    printf(" cypher[:10] = ");
    for (int i=0; i<10; i++)
	    printf(" %02x", cypher.bytes[i]);
    printf("\n");

    unsigned char cmd[3] = {1, 2, 3};
    unsigned int edc = calc_edc(cmd, 3);
    printf("\nedc is 0x%x\n", edc);

    return 0;


    int fd = open("/dev/sr0", O_RDONLY | O_NONBLOCK);

    //drive_info(fd, 1, true);
    drive_state(fd, false, 1, true);

    close(fd);

    return 0;
};
