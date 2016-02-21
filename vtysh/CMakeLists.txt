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

project("vtysh")

include(GNUInstallDirs)
include(FindPkgConfig)

pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
pkg_check_modules(OVSDB REQUIRED libovsdb)

add_definitions(-DHAVE_CONFIG_H -DSYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/lib
  ${PROJECT_SOURCE_DIR}
  ${OVSCOMMON_INCLUDE_DIRS}
  ${OPSCLI_INCLUDE_DIRS}
  )

add_executable(vtysh
  vtysh_main.c vtysh.c vtysh_user.c vtysh_config.c
  vtysh_ovsdb_if.c lldp_vty.c bgp_vty.c vrf_vty.c
  vtysh_ovsdb_config.c vtysh_ovsdb_config_context.c
  l3routes_vty.c system_vty.c
  vtysh_ovsdb_vrf_context.c
  sub_intf_vty.c loopback_vty.c
  vtysh_ovsdb_router_context.c aaa_vty.c
  logrotate_vty.c traceroute_vty.c
  traceroute_handler.c
  source_interface_selection_vty.c
  vtysh_ovsdb_source_interface_context.c
  vtysh_ovsdb_intf_lag_context.c
  lacp_vty.c ecmp_vty.c dhcp_tftp_vty.c
  vtysh_ovsdb_dhcp_tftp_context.c ping_vty.c
  ping_handler.c sftp_vty.c
  ospf_vty.c
  vtysh_ovsdb_sftp_context.c
  utils/vlan_vtysh_utils.c
  utils/intf_vtysh_utils.c
  utils/vrf_vtysh_utils.c)

target_link_libraries(vtysh PUBLIC
  ${OVSCOMMON_LIBRARIES}
  ${OVSDB_LIBRARIES}
  ${OPSCLI_LIBRARIES}
  crypt pthread readline ops-cli)

install(
  TARGETS vtysh
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}")
install(
  FILES
  vtysh_utils.h vtysh.h vtysh_ovsdb_if.h vtysh_user.h vtysh_ovsdb_config.h
  DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/vtysh")
install(
  FILES
  utils/system_vtysh_utils.h utils/vlan_vtysh_utils.h utils/lacp_vtysh_utils.h
  utils/intf_vtysh_utils.h utils/vrf_vtysh_utils.h
  DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/vtysh/utils")
