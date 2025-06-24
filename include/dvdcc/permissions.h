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
#ifndef DVDCC_PERMISSIONS_H_
#define DVDCC_PERMISSIONS_H_

#include <unistd.h>

namespace permissions {

void EnableRootPrivileges(void) {
  /* Raises privileges to root. This is needed to execute vendor
   * specific DVD commands like commands::ReadRawBytes()
   */
  if (getuid() != 0) seteuid(0);

}; // END permissions::EnableRootPrivileges()

void DisableRootPrivileges(void) {
  /* Drops privileges back to those of the real user.
   */
  uid_t user = getuid();
  uid_t effective_user = geteuid();

  if (user != 0 && user != effective_user) seteuid(user);

  return;

}; // END permissions::DisableRootPrivileges()

} // namespace permissions

#endif // DVDCC_PERMISSIONS_H_
