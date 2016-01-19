/* SHOW_TECH CLI commands.
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: show_tech_vty.h
 *
 * Purpose: To Run Show Tech command from CLI.
 */

#ifndef _SHOW_TECH_VTY_H
#define _SHOW_TECH_VTY_H

#define SHOW_TECH_STR              "Run show tech for all supported features\n"
#define SHOW_TECH_LIST_STR         "List all the supported show tech features\n"
#define SHOW_TECH_FEATURE_STR      "Run show tech for the feature specified\n"
#define SHOW_TECH_SUB_FEATURE_STR  "Run show tech for the sub feature specified\n"
void show_tech_vty_init();

#endif //_SHOW_TECH_VTY_H
