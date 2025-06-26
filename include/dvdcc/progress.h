/* Copyright (C) 2025     Josh Wood
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
#ifndef DVDCC_PROGRESS_H_
#define DVDCC_PROGRESS_H_

#include <time.h>
#include <stdio.h>
#include <unistd.h>

class Progress {

 public:
  void Start(void);
  void Update(int n, int total);
  void Finish(void);
  void DeltaString(char *buffer, double dt_sec);
   
  float dt;
  float frac;
  time_t t0;

  char bar[32];
  char elapsed[32];
  char remaining[32];

}; // END class Progress()

void Progress::Start(void) {
  /* Start the progress bar activity */
  t0 = time(NULL);
  sprintf(bar, "--------------------");

}; // END Progress::Start()

void Progress::Finish(void) {
  /* Finish the progress bar activity */
  printf("\n");
  fflush(stdout);

}; // END Progress::Finish()

void Progress::Update(int n, int total) {
  /* Update the progress bar
   *
   * Args:
   *     n (int): number of evaluated steps
   *     total (int): total steps
   */
  dt = difftime(time(NULL), t0);
  frac = float(n + 1) / total;

  for (int i = 0; i < 20 * (n + 1) / total; i++)
    bar[i] = '=';

  DeltaString(elapsed, dt);
  DeltaString(remaining, dt * (1/frac - 1));

  printf("\r\x1b[KProgress %s %5.1f%% | elapsed %s remaining %s ",
         bar, 100 * frac, elapsed, remaining);
  fflush(stdout);

}; // END Progress::Update()

void Progress::DeltaString(char *buffer, double dt_sec) {
  /* Form hour:minute:second string from a time delta in seconds.
   *
   * Args:
   *     buffer (char *): buffer for the output string
   *     dt_sec (double): time delta in seconds
   */
  int hr = dt_sec / 3600;
  int min = (dt_sec - 3600 * hr) / 60;
  int sec = dt_sec - 3600 * hr - 60 * min;

  if (hr <= 99)
    sprintf(buffer, "%02d:%02d:%02d", hr, min, sec);
  else
    sprintf(buffer, "XX:XX:XX");

}; // END Progress::DeltaString()

#endif // DVDCC_PROGRESS_H_
