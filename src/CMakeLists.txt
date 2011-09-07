#set(CMAKE_VERBOSE_MAKEFILE ON)

if(WIN32)
   set(VARIANT "windows")
   set(SUPPORT_TRACING 0)

   add_definitions(-DUNICODE)
   add_definitions(-D_UNICODE)
   add_definitions(-D_WIN32_WINNT=0x0501)

else(WIN32)
   set(VARIANT "linux")
   # DBus makes sense only on linux
   # WITH-DBUS == OFF means no DBus,
   #              TRY means autodetection
   #              ON means explicit support
   if(NOT DEFINED WITH-DBUS)
      set(WITH-DBUS "TRY" CACHE STRING "Build the DBus daemons (ON|TRY|OFF)")
   endif(NOT DEFINED WITH-DBUS)

   find_program(HELP2MAN help2man)
endif(WIN32)

option(WITH-QMF "Build the QMF daemon" ON)

set(CMAKE_C_FLAGS "-Wall -Wshadow")
set(CMAKE_CXX_FLAGS "-Wall -Wshadow")

set(AGENT_MAN_SECTION "8")
set(AGENT_MAN_DESC "Part of the Matahari suite of remote agents")

set(CONSOLE_MAN_SECTION "8")
set(CONSOLE_MAN_DESC "Console for the Matahari suite of remote agents")

set(DAEMON_TYPE "AGENT")

include(FindPkgConfig)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

file(GLOB SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")

# Configure checks
## Includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files (resolv.h HAVE_RESOLV_H)
include (CheckFunctionExists)
check_function_exists (asprintf HAVE_ASPRINTF)
check_function_exists (time HAVE_TIME)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

## Modules
# systemd
pkg_check_modules(systemd systemd)
if(systemd_FOUND)
    set(systemdunitdir "/lib/systemd/system")
endif(systemd_FOUND)

# pcre
pkg_check_modules(pcre REQUIRED libpcre)
if(NOT pcre_FOUND)
   if(MSVC)
        find_path(pcre_FOUND pcre.h "${PCRE_PATH}")
        find_library(pcre_LIBRARIES pcre.lib "${PCRE_PATH}")
   else(MSVC)
        find_path(pcre_FOUND pcre.h)
        find_library(pcre_LIBRARIES pcre)
        string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
   endif(MSVC)
   string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
endif(NOT pcre_FOUND)

if(NOT pcre_FOUND)
   message(FATAL_ERROR "PCRE header/library not found.")
endif(NOT pcre_FOUND)

# uuid
pkg_check_modules(uuid uuid)

# Sigar
find_library(SIGAR sigar)
if(SIGAR-NOTFOUND)
    message(FATAL_ERROR "SIGAR library not found.")
endif(SIGAR-NOTFOUND)

# GLib
pkg_check_modules(glib REQUIRED glib-2.0)
if(NOT glib_FOUND)
   message(FATAL_ERROR "Glib2 header/library not found.")
endif(NOT glib_FOUND)
include_directories(${glib_INCLUDE_DIRS})
SET(CMAKE_REQUIRED_LIBRARIES ${glib_LIBRARIES})
check_function_exists (g_list_free_full HAVE_G_LIST_FREE_FULL)

# cURL
if(NOT WIN32)
    find_library(CURL curl)
    if(CURL STREQUAL "CURL-NOTFOUND")
        message(FATAL_ERROR "libcurl not found.")
    endif(CURL STREQUAL "CURL-NOTFOUND")
endif(NOT WIN32)

# Include QMF and DBus generation macros
include(MatahariMacros)

## dbus
if(WITH-DBUS STREQUAL "ON")
    # Explicitly turned ON => fail when not found
    pkg_check_modules(dbus-glib REQUIRED dbus-glib-1)
    pkg_check_modules(polkit REQUIRED polkit-gobject-1)

    if(NOT dbus-glib_FOUND)
        message(FATAL_ERROR "dbus-glib headers/libraries not found.")
    endif(NOT dbus-glib_FOUND)

    if(NOT polkit_FOUND)
        message(FATAL_ERROR "polkit headers/libraries not found.")
    endif(NOT polkit_FOUND)
elseif(WITH-DBUS STREQUAL "OFF")
    # Explicitly turned OFF => no action
else(WITH-DBUS STREQUAL "ON")
    # All other options means TRY
    pkg_check_modules(dbus-glib dbus-glib-1)
    pkg_check_modules(polkit polkit-gobject-1)

    if(NOT dbus-glib_FOUND)
        message(STATUS "dbus-glib headers/libraries not found => DBus disabled")
        set(WITH-DBUS "OFF")
    endif(NOT dbus-glib_FOUND)

    if(NOT polkit_FOUND)
        message(STATUS "polkit headers/libraries not found => DBus disabled")
        set(WITH-DBUS "OFF")
    endif(NOT polkit_FOUND)
endif(WITH-DBUS STREQUAL "ON")

SET(CMAKE_REQUIRED_LIBRARIES ${polkit_LIBRARIES})
check_function_exists (polkit_authority_get_sync HAVE_PK_GET_SYNC)

## qpid
if(WITH-QMF)
    # Defines
    add_definitions(-DSUPPORT_TRACING=${SUPPORT_TRACING})
    add_definitions(-DMATAHARI_PORT=${MATAHARI_PORT})
    add_definitions(-DMATAHARI_BROKER="${MATAHARI_BROKER}")

    find_package(QPID REQUIRED)
    if(QPID-NOTFOUND)
        message(FATAL_ERROR "QPID library not found.")
    endif(QPID-NOTFOUND)

endif(WITH-QMF)


### Subdirectories
add_subdirectory(broker)
add_subdirectory(lib)
add_subdirectory(host)
add_subdirectory(network)
add_subdirectory(service)
add_subdirectory(sysconfig)
add_subdirectory(tests)

### Installation
install(FILES ${SCHEMAS} DESTINATION share/matahari)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h DESTINATION include)
install(FILES include/matahari/logging.h DESTINATION include/matahari)
install(FILES include/matahari/utilities.h DESTINATION include/matahari)
install(FILES include/matahari/dnssrv.h DESTINATION include/matahari)
install(FILES include/matahari/host.h DESTINATION include/matahari)
install(FILES include/matahari/network.h DESTINATION include/matahari)
install(FILES include/matahari/sysconfig.h DESTINATION include/matahari)
install(FILES include/matahari/services.h DESTINATION include/matahari)
install(FILES cmake/modules/FindMatahari.cmake DESTINATION share/cmake/Modules)
install(FILES cmake/modules/MatahariMacros.cmake DESTINATION share/cmake/Modules)

if(WITH-QMF)
    install(FILES cmake/modules/FindQPID.cmake DESTINATION share/cmake/Modules)
    install(FILES include/matahari/agent.h DESTINATION include/matahari)
    install(FILES include/matahari/mainloop.h DESTINATION include/matahari)
endif(WITH-QMF)

if(WITH-DBUS)
    install(FILES schema-to-dbus.xsl DESTINATION share/matahari)
    install(FILES dbus-to-c.xsl DESTINATION share/matahari)
    install(FILES check-policy.xsl DESTINATION share/matahari)
    install(FILES include/matahari/dbus_common.h DESTINATION include/matahari)
    install(FILES include/matahari/gobject_class.h DESTINATION include/matahari)
endif(WITH-DBUS)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/matahari.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h)

