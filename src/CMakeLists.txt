#set(CMAKE_VERBOSE_MAKEFILE ON)



if(WIN32)
   set(VARIANT "windows")
   set(SUPPORT_TRACING 0)

   add_definitions(-DUNICODE)
   add_definitions(-D_UNICODE)
   add_definitions(-D_WIN32_WINNT=0x0501)

else(WIN32)
   set(VARIANT "linux")
endif(WIN32)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")

set(MCOMMON_VERSION 0.0.1)
set(MHOST_VERSION 0.0.1)
set(MSRV_VERSION 0.0.1)
set(MNET_VERSION 0.0.1)

option(WITH-QMF "Build the QMF daemon" ON)
option(WITH-DBUS "Build the DBus daemon" ON)

include(FindPkgConfig)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/common")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

# set service name var's
set(M_HOST "matahari-hostd")
set(M_NET  "matahari-netd")
set(M_SRV  "matahari-serviced")

set(DAEMONS ${M_HOST} ${M_NET} ${M_SRV})

file(GLOB SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")

# Defines
add_definitions(-DSUPPORT_TRACING=${SUPPORT_TRACING})
add_definitions(-DMATAHARI_PORT=${MATAHARI_PORT})
add_definitions(-DMATAHARI_BROKER="${MATAHARI_BROKER}")

# Build targets
## Schema
set(SCHEMA_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Host.cpp
	${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Network.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Services.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Resources.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventHeartbeat.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventService_op.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/EventResource_op.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/QmfPackage.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/Package.cpp)

## Libraries and Common code
set(COMMON_SOURCES lib/mh_agent.cpp)

add_library (mcommon SHARED lib/utilities.c lib/mainloop.c)
set_target_properties(mcommon PROPERTIES SOVERSION ${MCOMMON_VERSION})

add_library (mhost SHARED lib/host.c lib/host_${VARIANT}.c)
set_target_properties(mhost PROPERTIES SOVERSION ${MHOST_VERSION})
target_link_libraries(mhost mcommon)

add_library (msrv SHARED lib/services.c lib/services_${VARIANT}.c)
set_target_properties(msrv PROPERTIES SOVERSION ${MSRV_VERSION})
target_link_libraries(msrv mcommon)

add_library (mnet SHARED lib/network.c lib/network_${VARIANT}.c)
set_target_properties(mnet PROPERTIES SOVERSION ${MNET_VERSION})
target_link_libraries(mnet mcommon)

if(WIN32)
   target_link_libraries(mhost wsock32)
   target_link_libraries(mcommon wsock32)
endif(WIN32)

## Daemons
add_executable(${M_HOST} host/matahari-host.cpp ${SCHEMA_SOURCES} ${COMMON_SOURCES} )
target_link_libraries(${M_HOST} mhost)

add_executable(${M_NET}  net/matahari-net.cpp  ${SCHEMA_SOURCES} ${COMMON_SOURCES})
target_link_libraries(${M_NET} mnet)

add_executable(${M_SRV}  service/matahari-srv.cpp ${SCHEMA_SOURCES} ${COMMON_SOURCES})
target_link_libraries(${M_SRV} msrv)

foreach(daemon ${DAEMONS}) 
   target_link_libraries(${daemon} mcommon)
   if(WIN32)
      target_link_libraries(${daemon} wsock32)
   endif(WIN32)
endforeach(daemon ${DAEMONS}) 

# Configure checks
## Includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (sys/ioctl.h HAVE_SYS_IOCTL_H)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/matahari.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h)

## Modules
pkg_check_modules(glib REQUIRED glib-2.0)
pkg_check_modules(pcre REQUIRED libpcre)
pkg_check_modules(dbus-glib REQUIRED dbus-glib-1)
pkg_check_modules(polkit REQUIRED polkit-gobject-1)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

if(NOT pcre_FOUND)
   IF(MSVC)
        find_path(pcre_FOUND pcre.h "${PCRE_PATH}")
        find_library(pcre_LIBRARIES pcre.lib "${PCRE_PATH}")
   ELSE(MSVC)
        find_path(pcre_FOUND pcre.h)
        find_library(pcre_LIBRARIES pcre)
	string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
   endif(MSVC)
   string(REPLACE "pcre.h" "" pcre_INCLUDE_DIRS ${pcre_FOUND})
endif(NOT pcre_FOUND)

if(pcre_FOUND)
   target_link_libraries(mhost ${pcre_LIBRARIES})
   include_directories(${pcre_INCLUDE_DIRS})
else(pcre_FOUND)
   message(FATAL_ERROR "PCRE header/library not found.")
endif(pcre_FOUND)

if(glib_FOUND)
   include_directories(${glib_INCLUDE_DIRS})
   foreach(daemon ${DAEMONS}) 
      target_link_libraries(${daemon} ${glib_LIBRARIES})
   endforeach(daemon ${DAEMONS}) 
   target_link_libraries(mcommon ${glib_LIBRARIES})
else(glib_FOUND)
   message(FATAL_ERROR "Glib2 header/library not found.")
endif(glib_FOUND)

## dbus

# Create src/dbus directory and generate DBus XML definition files into it
execute_process(COMMAND mkdir -p src/dbus)
find_file(XSLTPROC xsltproc)
execute_process(
    COMMAND ${XSLTPROC} ${CMAKE_CURRENT_SOURCE_DIR}/schema-to-dbus.xsl ${CMAKE_CURRENT_SOURCE_DIR}/schema.xml
    WORKING_DIRECTORY src/dbus
)


### DBus

add_subdirectory(host)
add_subdirectory(service)

## qpid

find_package(QPID REQUIRED)

## sigar
find_library(SIGAR sigar)
if(NOT_SIGAR)
   message(FATAL_ERROR "SIGAR library not found.")
endif(NOT_SIGAR)

target_link_libraries(mcommon ${SIGAR})
target_link_libraries(mhost   ${SIGAR})
target_link_libraries(mnet    ${SIGAR})

string(REGEX REPLACE "\\." "" QPID_VERSION ${QPID_VERSION})

message("Building against: ${QPID_VERSION_LONG} (${QPID_VERSION} )")
add_definitions(-DQPID_VERSION=${QPID_VERSION})

set(regen_schema OFF)
message(STATUS "Regenerating QMF schemas: ${SCHEMAS}")
foreach (schema_file ${SCHEMAS} ${QMFGEN})
    foreach (file ${SCHEMA_SOURCES})
        if (EXISTS ${file})
            if (${schema_file} IS_NEWER_THAN ${file})
                set(regen_schema ON)
	        message("${schema_file} is newer than ${file}")
            endif (${schema_file} IS_NEWER_THAN ${file})
        else (EXISTS ${file})
            set(regen_schema ON)
        endif (EXISTS ${file})
    endforeach (file ${SCHEMA_SOURCES})
endforeach (schema_file ${SCHEMAS})

if (regen_schema)
    # Regenerate everything 
    execute_process(COMMAND rm -f ${SCHEMA_SOURCES})
    execute_process(COMMAND ${QMFGEN} -o ./qmf ${SCHEMAS}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    execute_process(COMMAND ${QMFGEN} -2 -o ./qmf ${SCHEMAS}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else (regen_schema)
    message(STATUS "No need to generate QMF schema")
endif (regen_schema)

# Add QF libraries
foreach(daemon ${DAEMONS}) 
    target_link_libraries(${daemon} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    IF(QPIDTYPES_FOUND)
        target_link_libraries(${daemon} ${QPIDTYPES_LIBRARY})
    ENDIF(QPIDTYPES_FOUND)
endforeach(daemon ${DAEMONS}) 

# Install targets
install(TARGETS ${DAEMONS}  DESTINATION sbin)

if(WIN32)
install(TARGETS mcommon mhost mnet msrv DESTINATION sbin)
else(WIN32)
install(TARGETS mcommon mhost mnet msrv DESTINATION lib${LIB_SUFFIX})
endif(WIN32)

install(FILES ${SCHEMAS} DESTINATION share/matahari)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h DESTINATION include)
