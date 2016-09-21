/* Tunnel VTYSH utils
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tunnel_vtysh_utils.h"

void
get_mode_from_line(const char line[], char *mode_str)
{
  unsigned int count = 0, i = 0, j = 0, indx = 0;

  for(i=0; i<strlen(line); i++)
  {
    if(line[i]==0x20)
      count += 1;

    if(count >= 3)
    {
      for(j=i+1; j<strlen(line); j++)
      {
        indx = j-i-1;
        mode_str[indx] = line[j];
      }
      mode_str[j-i] = '\0';
      break;
    }
  }
}

int
get_id_from_name(const char *if_name)
{
    int id_number = 0;

    // Scans in the format of nameNumber, i.e. tunnel1 and extracts the integer
    if (!if_name || !sscanf(if_name, "%*[^0-9]" "%d", &id_number))
    {
        return -1;
    }

    return id_number;
}
