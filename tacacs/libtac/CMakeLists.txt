# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org>

cmake_minimum_required(VERSION 2.8)

project ("pam_tacplus")

set (LIBPAM_TACPLUS pam_tacplus)
set (LIBPAM_TACPLUS_STATIC pam_tacplus_s)

include(GNUInstallDirs)
include(FindPkgConfig)

# pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
# pkg_check_modules(OVSDB REQUIRED libovsdb)
# pkg_check_modules(OPSUTILS REQUIRED opsutils)

#add_definitions(-DHAVE_CONFIG_H -DSYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")
add_definitions(-DSYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -DFTR_PROXY_ARP -DFTR_LOCAL_PROXY_ARP -DHAVE_LIBCRYPTO -DHAVE_OPENSSL_MD5_H")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/tacacs/
  ${CMAKE_SOURCE_DIR}/tacacs/libtac/include
  ${CMAKE_SOURCE_DIR}/tacacs/libtac/lib
  ${PROJECT_SOURCE_DIR}
  )

# install(
#   FILES
#   support.h
#   libtac/include/cdefs.h
#   libtac/include/libtac.h
#   libtac/include/tacplus.h
#   libtac/lib/messages.h
#   libtac/lib/magic.h
#   libtac/lib/md5.h
#   libtac/lib/xalloc.h
#   pam_tacplus.h
#   DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/tacacs")
#

# CLI libraries source files
set (SOURCES_LIB_CLI
  ../pam_tacplus.c
  ../support.c
  lib/attrib.c
  lib/crypt.c
  lib/acct_s.c
  # lib/magic.c
  lib/authen_r.c
  lib/author_r.c
  lib/messages.c
  lib/author_s.c
  lib/connect.c
  lib/hdr_check.c
  lib/read_wait.c
  lib/acct_r.c
  lib/xalloc.c
  lib/cont_s.c
  #  lib/md5.c
  lib/authen_s.c
  lib/header.c
  lib/version.c
    )

add_library (${LIBPAM_TACPLUS} SHARED ${SOURCES_LIB_CLI})
add_library (${LIBPAM_TACPLUS_STATIC} STATIC ${SOURCES_LIB_CLI})


target_link_libraries(${LIBPAM_TACPLUS}
  util crypt pthread readline audit opsutils crypto pam pam_misc)

# target_link_libraries(${LIBPAM_TACPLUS_STATIC}
#   util crypt pthread readline ops-cli audit opsutils crypto pam pam_misc)
#
install(TARGETS ${LIBPAM_TACPLUS}
        LIBRARY DESTINATION lib/security
       )
# install(TARGETS ${LIBPAM_TACPLUS_STATIC}
#         LIBRARY DESTINATION lib/security
#        )
