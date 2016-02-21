#  Licensed under the Apache License, Version 2.0(the "License"); you may
#  not use this file except in compliance with the License. You may obtain
#  a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#  License for the specific language governing permissions and limitations
#  under the License.

cmake_minimum_required(VERSION 2.8)

project("ops-cli")
set(PROJECT_VERSION "0.99")
set(PROJECT_DESCRIPTION "OpenSwitch CLI library")

include(GNUInstallDirs)
include(FindPkgConfig)

pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
pkg_check_modules(OVSDB REQUIRED libovsdb)
pkg_check_modules(LIBCAP REQUIRED libcap)

add_definitions(-DHAVE_CONFIG_H -DSYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/lib
  ${PROJECT_SOURCE_DIR}
  ${OVSCOMMON_INCLUDE_DIRS})


add_custom_command(
  OUTPUT gitversion.h
  COMMAND perl gitversion.pl ${CMAKE_SOURCE_DIR} > gitversion.h 2> /dev/null
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
  OUTPUT route_types.h
  COMMAND perl route_types.pl < route_types.txt > route_types.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
  OUTPUT memtypes.h
  COMMAND gawk -f memtypes.awk memtypes.c > memtypes.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ops-cli SHARED
  gitversion.h route_types.h memtypes.h network.c getopt.c getopt1.c daemon.c
  checksum.c vector.c linklist.c vty.c command.c sockunion.c prefix.c thread.c
  if.c memory.c buffer.c table.c hash.c filter.c routemap.c distribute.c
  stream.c str.c log.c plist.c zclient.c sockopt.c smux.c agentx.c
  snmp.c md5.c if_rmap.c keychain.c privs.c sigevent.c pqueue.c jhash.c
  memtypes.c workqueue.c cli_plugins.c  lib_vtysh_ovsdb_if.c vty_utils.c)

target_link_libraries(ops-cli PUBLIC ${LIBCAP_LIBRARIES})
target_include_directories (ops-cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(ops-cli PROPERTIES VERSION "0")
set_target_properties(ops-cli PROPERTIES SOVERSION "0.0.0")

set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/vtysh")
set(PKG_CONFIG_LIBS "-L\${libdir} -lops-cli")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc")

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
  DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
install(
  TARGETS ops-cli
  LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(
  FILES
  buffer.h checksum.h command.h config.h distribute.h filter.h getopt.h
  hash.h if.h if_rmap.h jhash.h keychain.h libospf.h lib_vtysh_ovsdb_if.h
  linklist.h log.h md5.h memory.h network.h plist.h pqueue.h prefix.h
  privs.h routemap.h sigevent.h smux.h sockopt.h sockunion.h stream.h
  str.h table.h thread.h vector.h vty.h vty_utils.h workqueue.h zassert.h
  zclient.h zebra.h
  DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/vtysh")

# TODO(bluecmd): We're shipping version.h because projects like to include it
# but we're not shipping gitversion - so what's the point?
install(
  FILES version.h memtypes.h route_types.h
  DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/vtysh/lib")
