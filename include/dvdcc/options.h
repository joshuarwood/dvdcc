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
#ifndef DVDCC_OPTIONS_H_
#define DVDCC_OPTIONS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

class Options {
 public:
  Options()
    : load(0), eject(0), resume(0), verbose(0), iso(NULL), raw(NULL), device_path(NULL) {};
  ~Options() { free(iso); free(raw); free(device_path); };

  bool Parse(int argc, char **argv);
  void DisplayHelp(void) {
    printf("Usage: dvdcc --device DEVICE [--eject --load ...]\n"
           "Operate a DVD drive using SCSI commands.\n\n"
           "Command line options:\n"
           "  -d, --device      path to the device (example: /dev/sr0)\n"
           "      --eject       eject the disc\n"
           "      --load        load the disc\n"
           "  -i, --iso         create ISO backup\n"
           "  -r, --raw         create RAW backup\n"
           "      --resume      resume disc backup to existing file(s)\n"
           "      --verbose     print full command details\n"
           "      --help        display this help and exit\n");
  };

  int load;
  int eject;
  int resume;
  int verbose;

  char *iso;
  char *raw;
  char *device_path;

};

bool Options::Parse(int argc, char **argv) {

  // display help when no options are passed
  if (argc == 1) {
    DisplayHelp();
    exit(0);
  }

  // otherwise parse options
  while (1) {

    static struct option long_options[] = {
      {"help",    no_argument,       0,        'h'},
      {"device",  required_argument, 0,        'd'},
      {"eject",   no_argument,       &eject,   1},
      {"load",    no_argument,       &load,    1},
      {"iso",     required_argument, 0,        'i'},
      {"raw",     required_argument, 0,        'r'},
      {"resume",  no_argument,       &resume,  1},
      {"verbose", no_argument,       &verbose, 1},
      {0, 0, 0, 0}
    };

    int c = getopt_long(argc, argv, "hd:i:r:", long_options, NULL);

    if (c == -1)
      break;

    switch (c) {

      case 0:
        // used for flags like --eject, which automatically
        // set the corresponding value referenced in long_options
        break;

      case 'h':
        DisplayHelp();
        exit(0);
        break;

      case 'd':
        device_path = strdup(optarg);
        break;

      case 'i':
        iso = strdup(optarg);
        break;

      case 'r':
        raw = strdup(optarg);
        break;

      case '?':
        exit(1);
        break;

      default:
        abort();

    } // END switch (c)
  } // END while (1)

  if (device_path == NULL) {
    printf("dvdcc:options:Options:Parse() User must specific device path with --device.\n");
    printf("dvdcc:options:Options:Parse() Exiting...\n");
    exit(1);
  }

  return true;

}; // END Options::Parse()

#endif // DVDCC_OPTIONS_H_
