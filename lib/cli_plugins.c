/* Feature Specific CLI commands initialize via plugins source file.
 *
 * Copyright (C) 2015 Hewlett Packard Enterprise Development LP.
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
 * File: cli_plugins.c
 *
 * Purpose: To install the feature specific cli commands node & elements
 *          via plugins.
 */

#include <config.h>
#include <errno.h>
#include <ltdl.h>
#include <unistd.h>
#include <stdio.h>
#include "cli_plugins.h"
#include "openvswitch/vlog.h"

#define CLI_PLUGINS_ERR -1

VLOG_DEFINE_THIS_MODULE(cli_plugins);

typedef void(*plugin_func)(void);

/* plugin_class structure used for calling feature specific
 * cli init function by plugin_func.
 */
struct plugin_class {
    plugin_func    cli_init;
};

/*
 * Unique key to store and retrieve per-module data.
 */
static lt_dlinterface_id interface_id;

/*
 * Function : plugins_cli_destroy.
 * Responsibility : unloading all feature specific cli module.
 * Parameters : void.
 * Return : void.
 */
static
plugins_cli_destroy(void)
{
    lt_dlinterface_free(interface_id);
    lt_dlexit();
    VLOG_INFO("Destroyed all plugins");
    exit(1);
}


/*
 * Function : plugins_open_plugin.
 * Responsibility : load and call the feature specific module and init function.
 * Parameters :
 *   const char *filename: module filename which is passed by libltdl.
 *   void *data : contains module loading modes.which is initialized by libltdl.
 * Return : return 0 on suceess.
 */
static int
plugins_open_plugin(const char *filename, void *data)
{
    struct plugin_class plcl = {NULL};
    lt_dlhandle handle;

    if (!(handle = lt_dlopenadvise(filename, *(lt_dladvise *)data))) {
        fprintf(stderr, "Failed loading %s: %s\n", filename, lt_dlerror());
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }

    plcl.cli_init = lt_dlsym(handle, "cli_init");
    if (plcl.cli_init == NULL) {
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }


    if (lt_dlcaller_set_data(interface_id, handle, &plcl)) {
        fprintf(stderr, "plugin %s initialized twice\n", filename);
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }

    plcl.cli_init();

    VLOG_DBG("Loaded plugin library %s\n", filename);
    return 0;
}

/*
 * Function : plugins_cli_init.
 * Responsibility : Initialize feature specific cli commands.
 * Parameters : const char *path : libltdl search path.
 * Return : void.
 */
void
plugins_cli_init(const char *path)
{
     lt_dladvise advise;

    /* Initialize libltdl, set the libltdl serach path,
     * initialize advise parameter,which is used pass hints
     * to module loader when using lt_dlopenadvise to perform the loading.
     plugins_cli_init*/
    if (lt_dlinit() ||
        lt_dlsetsearchpath(path) ||
        lt_dladvise_init(&advise)) {
        VLOG_ERR("ltdl initializations: %s\n", lt_dlerror());
        return;
    }

    /* Register ops-cli interface validator with libltdl */
    if (!(interface_id = lt_dlinterface_register("ops-cli", NULL))) {
        VLOG_ERR("lt_dlinterface_register: %s\n", lt_dlerror());
        if (lt_dladvise_destroy(&advise)) {
            VLOG_ERR("destroying ltdl advise%s\n", lt_dlerror());
        }
        return;
    }

    /* set symglobal hint and call the feature specific cli init function via
     * pluginins_open_plugin function pointer.
     * 'lt_dlforeachfile' function will continue to make calls to
     * 'plugins_open_plugin()' for each file that it discovers in search_path
     * until one of these calls returns non-zero, or until the
     * files are exhausted.
     * `lt_dlforeachfile' returns value returned by the last call
     * made to 'plugins_open_plugin'.
     */
    if (lt_dladvise_global(&advise) || lt_dladvise_ext (&advise) ||
        lt_dlforeachfile(lt_dlgetsearchpath(), &plugins_open_plugin, &advise)) {
        VLOG_ERR("ltdl setting advise: %s\n", lt_dlerror());
        return;
    }

    VLOG_INFO("Successfully initialized all plugins");
    return;
}
