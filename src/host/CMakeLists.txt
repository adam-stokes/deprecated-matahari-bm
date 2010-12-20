# CMake stuff for host-dbus
#set(CMAKE_VERBOSE_MAKEFILE ON)

if(WIN32)
   set(VARIANT "windows")
else(WIN32)
   set(VARIANT "linux")
endif(WIN32)

set(CMAKE_C_FLAGS "-Wall -g -O2")

#set(MCOMMON_VERSION 0.0.1)
set(MHOST_VERSION 0.0.1)

include(FindPkgConfig)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")

# set service name var's
set(M_HOST_DBUS "matahari-host-dbusd")

#set(DAEMONS ${M_HOST_DBUS})

set(DBUS_INTROSPECT "${CMAKE_CURRENT_SOURCE_DIR}/matahari-host-dbus.xml")

# Build targets
## Schema
#set(SCHEMA_SOURCES
#        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Host.cpp
#	${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Network.cpp
#        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventHeartbeat.cpp
#        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventService_op.cpp
#        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventResource_op.cpp
#        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Package.cpp)

## Libraries and Common code
#set(COMMON_SOURCES lib/mh_agent.cpp)

#add_library (mcommon SHARED lib/utilities.c lib/mainloop.c)
#set_target_properties(mcommon PROPERTIES SOVERSION ${MCOMMON_VERSION})

#add_library (mhost SHARED lib/host.c lib/host_${VARIANT}.c)
#set_target_properties(mhost PROPERTIES SOVERSION ${MHOST_VERSION})
#target_link_libraries(mhost mcommon)

#add_library (msrv SHARED lib/services.c lib/services_${VARIANT}.c)
#set_target_properties(msrv PROPERTIES SOVERSION ${MSRV_VERSION})
#target_link_libraries(msrv mcommon)

#if(WIN32)
#   target_link_libraries(mhost wsock32)
#   target_link_libraries(mcommon wsock32)
#endif(WIN32)

## Daemons
#add_executable(${M_HOST} host/matahari-host.cpp ${SCHEMA_SOURCES} ${COMMON_SOURCES} )
#target_link_libraries(${M_HOST} mhost)

#add_executable(${M_NET}  net/matahari-net.cpp  ${SCHEMA_SOURCES} ${COMMON_SOURCES})

#add_executable(${M_SRV}  service/matahari-srv.cpp ${SCHEMA_SOURCES} ${COMMON_SOURCES})
#target_link_libraries(${M_SRV} msrv)

set(HOST_DBUS_SOURCE "matahari-host-dbus.c")

# Daemon
add_executable(${M_HOST_DBUS} ${HOST_DBUS_SOURCE})
target_link_libraries(${M_HOST_DBUS} mhost)

#foreach(daemon ${DAEMONS}) 
#   target_link_libraries(${daemon} mcommon)
#   if(WIN32)
#      target_link_libraries(${daemon} wsock32)
#   endif(WIN32)
#endforeach(daemon ${DAEMONS}) 

# Configure checks
## Includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (stdlib.h HAVE_STDLIB_H)
#check_include_files (glib-object.h HAVE_GLIB-OBJECT_H)
#check_include_files (glib/gi18n.h HAVE_GLIB_GI18N_H)
#check_include_files (dbus/dbus-glib-bindings.h HAVE_DBUS_DBUS-GLIB-BINDINGS_H)

#configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
#configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/matahari.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h)

## Modules
#pkg_check_modules(glib REQUIRED glib-2.0)
#pkg_check_modules(pcre REQUIRED libpcre)
#pkg_check_modules(netcf REQUIRED netcf)
pkg_check_modules(dbus-glib REQUIRED dbus-glib-1)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

#if(NOT pcre_FOUND)
#   IF(MSVC)
#        find_path(pcre_FOUND pcre.h "${PCRE_PATH}")
#        find_library(pcre_LIBRARIES pcre.lib "${PCRE_PATH}")
#   ELSE(MSVC)
#        find_path(pcre_FOUND pcre.h)
#        find_library(pcre_LIBRARIES pcre)
#	string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
#   endif(MSVC)
#   string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
#endif(NOT pcre_FOUND)

#if(pcre_FOUND)
#   target_link_libraries(mhost ${pcre_LIBRARIES})
#   include_directories(${pcre_INCLUDE_DIRS})
#else(pcre_FOUND)
#   message(FATAL_ERROR "PCRE header/library not found.")
#endif(pcre_FOUND)
#
#if(netcf_FOUND)
#   target_link_libraries(${M_NET} ${netcf_LIBRARIES})
#   include_directories(${netcf_INCLUDE_DIRS})
#else(netcf_FOUND)
#   message(FATAL_ERROR "NETCF header/library not found.")
#endif(netcf_FOUND)
#
#if(glib_FOUND)
#   include_directories(${glib_INCLUDE_DIRS})
#   foreach(daemon ${DAEMONS}) 
#      target_link_libraries(${daemon} ${glib_LIBRARIES})
#   endforeach(daemon ${DAEMONS}) 
#   target_link_libraries(mcommon ${glib_LIBRARIES})
#else(glib_FOUND)
#   message(FATAL_ERROR "Glib2 header/library not found.")
#endif(glib_FOUND)
if(dbus-glib_FOUND)
   include_directories(${dbus-glib_INCLUDE_DIRS})
   target_link_libraries(${M_HOST_DBUS} ${dbus-glib_LIBRARIES})
else(dbus-glib_FOUND)
   message(FATAL_ERROR "dbus-glib headers/libraries not found.")
endif(dbus-glib_FOUND)

## qpid

#if(MSVC)
#   find_file(QPIDD qpidd.exe "${QPID_PATH}")
#   find_path(QPID_INCLUDE Address.h "${QPID_PATH}")
#   find_library(QMFCONSOLE qmfconsole.lib "${QPID_PATH}")
#   find_library(QPIDCLIENT qpidclient.lib "${QPID_PATH}")
#   find_library(QPIDCOMMON qpidcommon.lib "${QPID_PATH}")
#   find_library(QPIDCOMMON qpidtypes.lib "${QPID_PATH}")
#
#   # Figure out some way to do this automatically on windows
#   # Perhaps with the strings command
#   set(QPID_VERSION_LONG "qpidd (qpidc) version 0.6")
#
#else(MSVC)
#   find_library(QMF qmf)
#   find_library(QPIDCLIENT qpidclient)
#   find_library(QPIDCOMMON qpidcommon)	
#   find_library(QPIDTYPES qpidtypes)	
#
#   # Figure out the qpid version
#   if(WIN32)
#      find_file(QPIDD qpidd.exe)
#      execute_process(COMMAND strings ${QPIDD} OUTPUT_VARIABLE QPID_VERSION_LONG)
#      string(REGEX REPLACE ".*BUILD/qpid" " qpid" QPID_VERSION_LONG ${QPID_VERSION_LONG})
#      string(REGEX REPLACE "/cpp.*" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})
#
#      #version: qpid-cpp-0.6.895736
#      string(REGEX REPLACE "qpid-cpp-" "" QPID_VERSION ${QPID_VERSION_LONG})
#      string(REGEX REPLACE ".[0-9]*$" "" QPID_VERSION ${QPID_VERSION})
#
#   else(WIN32)
#      find_file(QPIDD qpidd)
#      execute_process(COMMAND ${QPIDD} --version  OUTPUT_VARIABLE QPID_VERSION_LONG)
#      string(STRIP QPID_VERSION_LONG ${QPID_VERSION_LONG})
#      string(REGEX REPLACE "d \\(.*\\)" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})
#      string(REGEX REPLACE ".* " "" QPID_VERSION ${QPID_VERSION_LONG})
#   endif(WIN32)
#endif(MSVC)
#
#if(NOT QMF)
#   message(FATAL_ERROR "QMF library not found.")
#endif(NOT QMF)
#
#if(NOT QPIDCLIENT)
#   message(FATAL_ERROR "QPIDCLIENT library not found.")
#endif(NOT QPIDCLIENT)
#
#if(NOT QPIDCOMMON)
#   message(FATAL_ERROR "QPIDCOMMON library not found.")
#endif(NOT QPIDCOMMON)
#
#if(NOT QPIDTYPES)
#   message("QPIDTYPES library not found.")
#   set(QPIDTYPES "")
#endif(NOT QPIDTYPES)
#
#find_program(QMFGEN qmf-gen)
#if(QMFGEN-NOTFOUND)
#   message(FATAL_ERROR Missing QMF schema processor.)
#endif(QMFGEN-NOTFOUND)
#
#string(REGEX REPLACE "\\." "" QPID_VERSION ${QPID_VERSION})
#
#message("Building against: ${QPID_VERSION_LONG} (${QPID_VERSION} )")
#add_definitions(-DQPID_VERSION=${QPID_VERSION})
#
#set(regen_schema OFF)
#message(STATUS "Regenerating QMF schemas: ${SCHEMAS}")
#foreach (schema_file ${SCHEMAS} ${QMFGEN})
#    foreach (file ${SCHEMA_SOURCES})
#        if (EXISTS ${file})
#            if (${schema_file} IS_NEWER_THAN ${file})
#                set(regen_schema ON)
#	        message("${schema_file} is newer than ${file}")
#            endif (${schema_file} IS_NEWER_THAN ${file})
#        else (EXISTS ${file})
#            set(regen_schema ON)
#        endif (EXISTS ${file})
#    endforeach (file ${SCHEMA_SOURCES})
#endforeach (schema_file ${SCHEMAS})
#
#if (regen_schema)
#    # Regenerate everything 
#    execute_process(COMMAND rm -f ${SCHEMA_SOURCES})
#    execute_process(COMMAND ${QMFGEN} -o ./qmf ${SCHEMAS}
#                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
#else (regen_schema)
#    message(STATUS "No need to generate QMF schema")
#endif (regen_schema)
#
## Add QF libraries
#foreach(daemon ${DAEMONS}) 
#    target_link_libraries(${daemon} ${QPIDCOMMON} ${QPIDCLIENT} ${QPIDTYPES} ${QMF})
#endforeach(daemon ${DAEMONS}) 

# DBus
find_file(BIND_TOOL dbus-binding-tool)
execute_process(COMMAND ${BIND_TOOL}
                --prefix=fmci --mode=glib-server
                --output=matahari-host-dbus-glue.h ${DBUS_INTROSPECT}
#                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/fmci})
)

# Install targets
#install(TARGETS ${M_HOST}  DESTINATION sbin)
#install(TARGETS ${M_NET}   DESTINATION sbin)
install(TARGETS ${M_HOST_DBUS} DESTINATION sbin)

#install(TARGETS mcommon mhost msrv DESTINATION lib)
#install(TARGETS mhost DESTINATION lib)

#install(FILES ${SCHEMAS} DESTINATION share/matahari)
#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari.h DESTINATION include)

# TODO: fix install paths
install(FILES ${DBUS_INTROSPECT} DESTINATION share/dbus-1/interfaces)
install(FILES org.matahariproject.Host.conf /etc/dbus-1/system.d)