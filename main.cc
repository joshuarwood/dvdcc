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
#include <unistd.h>
#include "dvdcc/options.h"
#include "dvdcc/constants.h"
#include "dvdcc/cypher.h"
#include "dvdcc/progress.h"
#include "dvdcc/devices.h"
#include "dvdcc/ecma_267.h"
#include "dvdcc/commands.h"
#include <iostream>

int main(int argc, char **argv) {

  // welcome message
  printf("dvdcc version 0.2.0, Copyright (C) 2025 Josh Wood\n"
         "dvdcc comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.\n"
         "This is free software, and you are welcome to redistribute it\n"
         "under certain conditions; see LICENSE for details.\n\n");

  // parse command line options
  Options options;
  options.Parse(argc, argv);

  // open drive with 1 second timeout setting
  Dvd dvd(options.device_path, 1);
  printf("\nFound drive model: %s\n", dvd.model);

  // mutually exclusive load/eject commands
  if (options.load) {
    dvd.Load(options.verbose);
    return 0;
  } else if (options.eject) {
    dvd.Eject(options.verbose);
    return 0;
  }

  // should start with a test ready check loop
  // should also test sequential blocks

  //commands::AbortTest(dvd.fd, dvd.timeout, true, NULL);
  return 0;
  //dvd.Stop();
  //sleep(1);
  dvd.Start();
  //dvd.ClearSectorCache(0);
  const int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;
  unsigned char buffer[buflen];
  dvd.ReadRawSectorCache(0, buffer);
  dvd.ReadRawSectorCache(10000, buffer);
  dvd.ReadRawSectorCache(0, buffer, true);
  unsigned int sector0 = dvd.RawSectorId(buffer);
  /*for (int i=0; i<80; i++) {
    unsigned int sector = dvd.RawSectorId(buffer + i * 2064);
    printf("sector %d\n", sector - sector0);
    printf(" %02x %02x %02x %02x\n", buffer[i * 2064], buffer[i*2064+1], buffer[i*2064+2], buffer[i*2064+3]);
  }*/
  return 0;
  /*
  //dvd.Stop(true);
  commands::AbortTest(dvd.fd, dvd.timeout, true, NULL); 
  return 0;
  sleep(1);
  dvd.Start(true);
  sleep(1);
  dvd.Stop(true);
  sleep(1);
  dvd.Start(true);
  return 0;
  //dvd.Stop(true);
  //return 0;
  */
  /*
  for (int i=0; i <10; i++) {
    dvd.Stop(true);
    sleep(1);
  }
  return 0;*/
  dvd.Start(true);
  dvd.FindDiscType(true);
  dvd.FindKeys(20, true) == 0;
  return 0;

  int retry = 0;
  while (true) {
    // stop when we find all keys
    if (dvd.FindKeys(20, true) == 0)
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
