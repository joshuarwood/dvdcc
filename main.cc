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
  printf("Found drive model: %s\n", dvd.model);

  // mutually exclusive load/eject commands
  if (options.load) {
    printf("\nLoading disc...\n\n");
    dvd.Load(options.verbose);
    printf("Done.\n");
    return 0;
  } else if (options.eject) {
    printf("\nEjecting disc...\n\n");
    dvd.Eject(options.verbose);
    printf("Done.\n");
    return 0;
  }

  int retry = 0;
  printf("\nChecking if drive is ready...\n");
  while ((dvd.PollReady(options.verbose) != 0) ||
         (dvd.PollPowerState(options.verbose) == (int)constants::PowerStates::kActive)) {
    if (++retry == 1) {
      printf("Waiting for activity to stop...\n");
      fflush(stdout);
    } else if (retry == 1000) {
      printf("\n\ndvdcc:main() Drive activity did not stop after 1000 seconds.");
      printf("\ndvdcc:main() Exiting...\n");
      exit(0);
    }
    sleep(1);
  } // END while (dvd.PollPower...)
  printf("Ready.\n");

  return 0;


  // should start with a test ready check loop
  // should also test sequential blocks

  // start spinning the disc and determine disc type
  dvd.Start(options.verbose);
  dvd.FindDiscType(options.verbose);

  // find the keys needed to decode disc data
  retry = 0;
  while (true) {
    // stop when we find all keys
    if (dvd.FindKeys(20, options.verbose) == 0)
      break;

    // otherwise try to flush current cache and retry
    dvd.ClearSectorCache(0, options.verbose);
    if (retry++ == 5) {
      printf("dvdcc:main() Reached maximum retry for FindKeys().\n");
      printf("dvdcc:main() Exiting...\n");
      return 0;
    } // END if (retry)

  } // END while (true)

  // display full disc info
  dvd.DisplayMetaData();

  return 0;

};
