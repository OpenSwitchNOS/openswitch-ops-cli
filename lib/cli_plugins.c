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
 * File: cli_plugins.c
 *
 * Purpose: To install the feature specific cli commands node & elements
 *          via plugins.
 */

#include <config.h>
#include <errno.h>
#include <ltdl.h>
#include <unistd.h>
#include "cli_plugins.h"
#include "openvswitch/vlog.h"
#include <string.h>
#include <stdbool.h>


#define PLUGINS_CALL(FUN) \
do { \
    lt_dlhandle iter_handle = 0; \
    struct plugin_class plcl; \
    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) { \
        plcl.cli_init = lt_dlsym(iter_handle, #FUN); \
        if (plcl.cli_init != NULL) \
            plcl.cli_init(); \
    } \
}while(0)


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

/* List of disabled CLI plugins */
static char *cli_disable_plugins[] = {
  "switchd",
  NULL
};

/*
 * Function : plugins_check_if_plugin_is_disabled.
 * Responsibility : Check if the feature specific module is allowed
 *                  to load
 * Parameters :
 *   const char *filename: Module filename which is passed by libltdl.
 * Return : Return true if plugin should be disabled, or
 *          Return false if plugin should be enabled
 */
static bool
plugins_check_if_plugin_is_disabled(const char *filename)
{
    int plugin_index = 0;
    while (cli_disable_plugins[plugin_index] != NULL) {
        if(strstr(filename, cli_disable_plugins[plugin_index]) != NULL) {
            return true;
        }
        plugin_index++;
    }
    return false;
}

/*
 * Function : plugins_cli_destroy.
 * Responsibility : Unloading all feature specific cli module.
 * Parameters : void.
 * Return : void.
 */
static void
plugins_cli_destroy(void)
{
    lt_dlinterface_free(interface_id);
    lt_dlexit();
    VLOG_INFO("Destroyed all plugins");
    exit(1);
}

/*
 * Function : plugins_open_plugin.
 * Responsibility : Load and call the feature specific module and cli node
 *                  init function.
 * Parameters :
 *   const char *filename: Module filename which is passed by libltdl.
 *   void *data : It contains module loading modes,
 *                which is initialized by libltdl.
 * Return : Return 0 on success.
 */
static int
plugins_open_plugin(const char *filename, void *data)
{
    struct plugin_class plcl = {NULL};
    lt_dlhandle handle;

    if (plugins_check_if_plugin_is_disabled(filename)) {
        VLOG_DBG("Plugin %s is disabled\n",filename);
        return CLI_PLUGINS_SUCCESS;
    }

    if (!(handle = lt_dlopenadvise(filename, *(lt_dladvise *)data))) {
        VLOG_ERR("Failed loading %s: %s\n", filename, lt_dlerror());
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }

    plcl.cli_init = lt_dlsym(handle, "cli_pre_init");
    if (plcl.cli_init == NULL) {
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }


    if (lt_dlcaller_set_data(interface_id, handle, &plcl)) {
        VLOG_ERR("plugin %s initialized twice\n", filename);
        plugins_cli_destroy();
        return CLI_PLUGINS_ERR;
    }

    plcl.cli_init();

    VLOG_DBG("Loaded plugin library %s\n", filename);
    return CLI_PLUGINS_SUCCESS;
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

    /* Initialize libltdl, set libltdl search path.
     * Initialize advise parameter,which is used to pass hints
     * to module loader when using lt_dlopenadvise to perform the loading.
     */
    if (lt_dlinit() ||
        lt_dlsetsearchpath(path) ||
        lt_dladvise_init(&advise)) {
        VLOG_ERR("ltdl initializations: %s\n", lt_dlerror());
        return;
    }

    /* Register ops-cli interface validator with libltdl. */
    if (!(interface_id = lt_dlinterface_register("ops-cli", NULL))) {
        VLOG_ERR("lt_dlinterface_register: %s\n", lt_dlerror());
        if (lt_dladvise_destroy(&advise)) {
            VLOG_ERR("destroying ltdl advise%s\n", lt_dlerror());
        }
        return;
    }

    /* Set symglobal hint and call the feature specific cli init function via
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

/*
 * Function : vtysh_cli_post_init.
 * Responsibility : Initialize all feature specific cli elements.
 * Parameters : void.
 * Return : void.
 */
void
vtysh_cli_post_init(void)
{
    PLUGINS_CALL(cli_post_init);
}
