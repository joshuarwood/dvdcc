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
  Dvd dvd(options.device_path, 1, options.verbose);
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

  printf("\nChecking if drive is ready...\n\n");

  Progress progress("Waiting for standby state...", true);
  progress.Start();

  // make sure we wait for drive activity to stop before continuing,
  // otherwise background commands might overwrite the drive cache
  // as we try to read it
  int retry = 0, good = 0;
  while (true) {

    bool ready = (dvd.PollReady(options.verbose) == 0);
    bool active = (dvd.PollPowerState(options.verbose) == (int)constants::PowerStates::kActive);

    good += (ready && !active);
    retry++;

    // only break after verifying the drive is ready 3 consecutive times
    // to avoid triggering on the transition between unready and active states
    if (good == 3) break;

    if (retry != good) progress.Update();

    if (retry == 1000) {
      progress.Finish();
      printf("\n\ndvdcc:main() Drive activity did not stop after 1000 seconds.");
      printf("\ndvdcc:main() Exiting...\n");
      exit(0);
    }
    sleep(1);

  } // END while (dvd.PollPower...)

  // add back white space that was over-written by progress
  if (retry > good) printf("\n\n");

  // start spinning the disc and determine disc type
  dvd.Start(options.verbose);
  dvd.FindDiscType(options.verbose);

  // find the keys needed to decode disc data
  retry = 0;
  while (true) {
    // stop when we find all keys
    if (dvd.FindKeys(20, options.verbose) == 0)
      break;

    // enter standy to try flushing cache
    dvd.Stop();
    dvd.ClearSectorCache(0, options.verbose);
    if (retry++ == 5) {
      printf("dvdcc:main() Reached maximum retry for FindKeys().\n");
      printf("dvdcc:main() Exiting...\n");
      return 0;
    } // END if (retry)

  } // END while (true)

  // display full disc info
  dvd.DisplayMetaData();

  // break here if no backup is requested
  if (!options.iso && !options.raw)
    return 0;

  printf("Backing up content...\n\n");

  // open file for iso backup
  FILE *fiso;
  if (options.iso) {
    printf(" ISO path: %s\n", options.iso);
    if (options.resume == 0 && access(options.iso, F_OK) == 0) {
      printf("dvdcc:main() File already exists. Delete or use --resume.\n");
      printf("dvdcc:main() Exiting...\n");
      return 0;
    }
    fiso = fopen(options.iso, options.resume ? "a+b" : "wb");
  } // END if (options.iso)

  // open file for raw backup
  FILE *fraw;
  if (options.raw) {
    printf(" RAW path: %s\n", options.raw);
    if (options.resume == 0 && access(options.iso, F_OK) == 0) {
      printf("dvdcc:main() File already exists. Delete or use --resume.\n");
      printf("dvdcc:main() Exiting...\n");
      return 0;
    }
    fraw = fopen(options.raw, options.resume ? "a+b" : "wb");
  } // END if (options.raw)
  printf("\n");

  // backup loop variables
  const int buflen = constants::RAW_SECTOR_SIZE * constants::SECTORS_PER_CACHE;
  unsigned char buffer[buflen];
  unsigned char *raw_sector;
  unsigned int i, raw_edc, edc_length = constants::RAW_SECTOR_SIZE - 4;

  // determine starting point based on resume option
  unsigned int start_sector = 0;
  if (options.resume) {
    if (options.iso) { // get start sector from iso when present
      fseek(fiso, 0, SEEK_END);
      start_sector = ftell(fiso) / constants::SECTOR_SIZE;
    } else {
      fseek(fraw, 0, SEEK_END);
      start_sector = ftell(fraw) / constants::RAW_SECTOR_SIZE;
    }
  } // END if (options.resume)

  // prepare progress tracker
  strcpy(progress.description, "Progress");
  progress.only_elapsed = false;
  progress.Start();

  // loop through dvd sectors
  for (unsigned int sector = start_sector; sector < dvd.sector_number; sector++) {

    // perform cache read if this is the start of a cache block or a resume
    if ((sector % constants::SECTORS_PER_CACHE == 0) || options.resume) {
      dvd.ReadRawSectorCache(sector, buffer, options.verbose);
      options.resume = 0;
    }

    // get the cypher index for decoding this sector
    i = dvd.CypherIndex(sector / constants::SECTORS_PER_BLOCK);

    // point to the raw sector data
    raw_sector = buffer + sector % constants::SECTORS_PER_CACHE * constants::RAW_SECTOR_SIZE;

    // try decoding the raw sector data
    for (retry = 0; retry < 20; retry++) {
      
      dvd.cyphers[i]->Decode64(raw_sector, 12);

      raw_edc = dvd.RawSectorEdc(raw_sector);

      if (raw_edc == ecma_267::calculate(raw_sector, edc_length)) {
        if (options.iso) fwrite(raw_sector + 6, 1, constants::SECTOR_SIZE, fiso);
        if (options.raw) fwrite(raw_sector, 1, constants::RAW_SECTOR_SIZE, fraw);
        break;
      }

      printf("\r\x1b[KRetrying sector %lu (attempt %d)\n", sector, retry+1);

      if (retry == 19) {
        printf("dvdcc:main() Cannot read sector %lu\n", sector);
        printf("dvdcc:main() Exiting...\n");
        return 1;
      }

      // decode failed so retry after clearing cache
      dvd.Stop();
      dvd.ClearSectorCache(sector, options.verbose);
      dvd.ReadRawSectorCache(sector, buffer, options.verbose);

    } // END for (retry)

    progress.Update(sector, dvd.sector_number);

  } // END for (sector)
  progress.Finish();

  // close files
  if (options.iso) fclose(fiso);
  if (options.raw) fclose(fraw);

  return 0;

}; // END main()
