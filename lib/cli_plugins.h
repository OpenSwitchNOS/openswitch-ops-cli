/* Feature Specific CLI commands initialize via plugins source file.
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: cli_plugins.h
 *
 * Purpose: To install the feature specific cli commands node & elements
 *          via plugins.
 */


#ifndef CLI_PLUGINS_H
#define CLI_PLUGINS_H 1

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef __linux__
void plugins_cli_init(const char *path);
#endif

#ifdef  __cplusplus
}
#endif

#endif /* cli_plugins.h */
