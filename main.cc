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

#include <time.h>
#include "dvdcc/constants.h"
#include "dvdcc/cypher.h"
#include "dvdcc/progress.h"
#include "dvdcc/devices.h"
#include "dvdcc/ecma_267.h"
#include "dvdcc/commands.h"
#include <iostream>

int main(void) {

  // open drive
  Dvd dvd("/dev/sr0");
  printf("\nFound drive model: %s\n", dvd.model);

  //dvd.Stop(true);
  dvd.Start();
  dvd.FindDiscType();

  int retry = 0;
  while (true) {
    // stop when we find all keys
    if (dvd.FindKeys() == 0)
      break;

    // otherwise try to flush current cache and retry
    dvd.ClearSectorCache(0);
    if (retry++ == 3) {
      printf("dvdcc:main() Reached maximum retry for FindKeys().\n");
      printf("dvdcc:main() Exiting...\n");
      return 0;
    } // END if (retry)

  } // END while (true)

  dvd.DisplayMetaData();
  return 0;

  /*
  // prepare to read
  FILE *f = fopen("test.bin", "wb");
  unsigned char buffer[constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE];

  // start the progress bar
  Progress progress;
  progress.Start();

  // read sectors and write to file
  dvd.Start(true);
  for (int sector = 0; sector < constants::disc_sector_no["GAMECUBE"]; sector += constants::SECTORS_PER_CACHE) {
    dvd.ReadRawSectorCache(sector, buffer, false);
    fwrite(buffer, 1, sizeof(buffer), f);
    if (sector % 100 == 0)
        progress.Update(sector, constants::disc_sector_no["GAMECUBE"]);
    if (sector > 500)
	 break;
  }
  progress.Finish();
  fclose(f);
*/
};
