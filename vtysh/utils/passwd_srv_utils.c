/*
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <ltdl.h>

#include "openvswitch/vlog.h"
#include "command.h"
#include "passwd_srv_utils.h"

VLOG_DEFINE_THIS_MODULE(passwd_srv_utils);

static char sock_path[PASSWD_SRV_MAX_STR_SIZE] = {0};
static char pub_key_path[PASSWD_SRV_MAX_STR_SIZE] = {0};
static int  initialized = 0;

/**
 * Get socket descriptor path to connect with the password server
 *
 * @return path to socket descriptor
 */
char *get_passwd_sock_fd_path()
{
    return (initialized)?sock_path:NULL;
}

/**
 * Get public key path to encrypt message
 *
 * @return path to socket descriptor
 */
char *get_passwd_pub_key_path()
{
    return (initialized)?pub_key_path:NULL;
}

/**
 * Load password shared object to retrieve socket/public-key file path
 * from yaml file
 *
 * @return 0 if file paths are retrieved successfully
 */
int passwd_srv_path_manager_init()
{
    lt_dlhandle so_handle = 0;
    int (*init_ptr)(void) = NULL; /* function pointer to call init funtion */
    char *(*get_path_ptr)(void) = NULL; /* function pointer to get path */
    char *path_name = NULL;

    lt_dlinit();
    lt_dlerror();

    /* open shared object to be used */
    so_handle = lt_dlopen(PASSWD_SRV_SO_LIB);

    if (lt_dlerror())
    {
        VLOG_ERR ("Failed to load the password server library");
        return -1;
    }

    init_ptr = lt_dlsym(so_handle, "init_yaml_parser");

    if ((lt_dlerror()) || (NULL == init_ptr))
    {
        VLOG_ERR ("Failed to find init_yaml_parser");
        lt_dlclose(so_handle);
        return -1;
    }

    if (PASSWD_ERR_SUCCESS != init_ptr())
    {
        VLOG_ERR ("Failed to parse yaml file");
        lt_dlclose(so_handle);
        return -1;
    }

    get_path_ptr = lt_dlsym(so_handle, "get_socket_descriptor_path");

    if ((lt_dlerror()) || (NULL == init_ptr))
    {
        VLOG_ERR ("Failed to find get_socket_descriptor_path");
        lt_dlclose(so_handle);
        return -1;
    }

    /* get socket descriptor path */
    if ((NULL == (path_name = get_path_ptr())) ||
            (PASSWD_SRV_MAX_STR_SIZE < strlen(path_name)))
    {
        VLOG_ERR ("Failed to get socket fd path");
        lt_dlclose(so_handle);
        return -1;
    }
    else
    {
        /* copy socket fd path */
        memcpy(sock_path, path_name, strlen(path_name));
        path_name = NULL;
    }

    get_path_ptr = lt_dlsym(so_handle, "get_public_key_path");

    if ((lt_dlerror()) || (NULL == init_ptr))
    {
        VLOG_ERR ("Failed to find get_public_key_path");
        lt_dlclose(so_handle);
        return -1;
    }

    /* get public key path */
    if ((NULL == (path_name = get_path_ptr())) ||
            (PASSWD_SRV_MAX_STR_SIZE < strlen(path_name)))
    {
        VLOG_ERR ("Failed to get public key path");
        lt_dlclose(so_handle);
        return -1;
    }
    else
    {
        /* copy socket fd path */
        memcpy(pub_key_path, path_name, strlen(path_name));
    }

    init_ptr = lt_dlsym(so_handle, "uninit_yaml_parser");

    if ((lt_dlerror()) || (NULL == init_ptr))
    {
        VLOG_ERR ("Failed to find init_yaml_parser");
        lt_dlclose(so_handle);
        return -1;
    }

    if (PASSWD_ERR_SUCCESS != init_ptr())
    {
        VLOG_ERR ("Failed to uninit yaml parser");
        lt_dlclose(so_handle);
        return -1;
    }

    initialized = 1;
    lt_dlclose(so_handle);
    return 0;
}
