#set(CMAKE_VERBOSE_MAKEFILE ON)

include(FindPkgConfig)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/qmf")

# set service name var's
set(M_HOST "matahari-hostd")
set(M_NET  "matahari-netd")
set(M_PKG  "matahari-packaged")

file(GLOB SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")

# Defines
add_definitions(-DMATAHARI_PORT=${MATAHARI_PORT})
add_definitions(-DMATAHARI_BROKER="${MATAHARI_BROKER}")

# Build targets
## Host
set(HOST_SOURCES
        matahari-host.cpp
        qmf/hostagent.cpp)

# TODO: host.cpp and processor.cpp need to be re-written in C
#       and then combined with util.c into a library/dll
#
#       Refer to qpid CMake control file for how to do this
set(HOST_LIBRARY
        host.cpp
        processor.cpp
        util.c)

set(HOST_SCHEMA_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/com/redhat/matahari/host/Package.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/com/redhat/matahari/host/Host.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/qmf/com/redhat/matahari/host/EventHeartbeat.cpp)

add_executable(${M_HOST} ${HOST_SOURCES} ${HOST_LIBRARY} ${HOST_SCHEMA_SOURCES})

## Network
set(NET_SOURCES
	matahari-net.cpp
	qmf/netagent.cpp)

set(NET_SCHEMA_SOURCES
	${CMAKE_CURRENT_BINARY_DIR}/qmf/com/redhat/matahari/net/Package.cpp
	${CMAKE_CURRENT_BINARY_DIR}/qmf/com/redhat/matahari/net/Network.cpp)

add_executable(${M_NET} ${NET_SOURCES} ${NET_SCHEMA_SOURCES})

# Configure checks
## Includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (sys/ioctl.h HAVE_SYS_IOCTL_H)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/matahari.h.in ${CMAKE_CURRENT_BINARY_DIR}/matahari.h)

## Modules
pkg_check_modules(pcre REQUIRED libpcre)
pkg_check_modules(netcf REQUIRED netcf)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

if(WIN32)
   target_link_libraries(${M_HOST} wsock32)
else(WIN32)
   find_library(VIRT virt)
   if(VIRT_FOUND)
	target_link_libraries(${M_HOST} ${VIRT})
   endif(VIRT_FOUND)
endif(WIN32)

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
   target_link_libraries(${M_HOST} ${pcre_LIBRARIES})
   include_directories(${pcre_INCLUDE_DIRS})
else(pcre_FOUND)
   message(FATAL_ERROR "PCRE header/library not found.")
endif(pcre_FOUND)

if(netcf_FOUND)
   target_link_libraries(${M_NET} ${netcf_LIBRARIES})
   include_directories(${netcf_INCLUDE_DIRS})
else(netcf_FOUND)
   message(FATAL_ERROR "NETCF header/library not found.")
endif(netcf_FOUND)

set(simple_libs check libvirt udev_new)
foreach(lib ${simple_libs}) 
    find_library(${lib}_LIBRARIES ${lib})
    if(${lib}_LIBRARIES) 
        target_link_libraries(${M_HOST} ${${lib}_LIBRARIES})
    else(${lib}_LIBRARIES) 
    	message("Library ${lib} not found")
    endif(${lib}_LIBRARIES) 
endforeach(lib ${simple_libs})

## qpid

if(MSVC)
   find_file(QPIDD qpidd "${QPID_PATH}")
   find_path(QPID_INCLUDE Address.h "${QPID_PATH}")
   find_library(QMFCONSOLE qmfconsole.lib "${QPID_PATH}")
   find_library(QPIDCLIENT qpidclient.lib "${QPID_PATH}")
   find_library(QPIDCOMMON qpidcommon.lib "${QPID_PATH}")
else(MSVC)
   find_file(QPIDD qpidd)
   find_library(QMF qmf)
   find_library(QPIDCLIENT qpidclient)
   find_library(QPIDCOMMON qpidcommon)	
endif(MSVC)

if(NOT QMF)
   message(FATAL_ERROR "QMF library not found.")
endif(NOT QMF)

if(NOT QPIDCLIENT)
   message(FATAL_ERROR "QPIDCLIENT library not found.")
endif(NOT QPIDCLIENT)

if(NOT QPIDCOMMON)
   message(FATAL_ERROR "QPIDCOMMON library not found.")
endif(NOT QPIDCOMMON)

find_program(QMFGEN qmf-gen)
if(QMFGEN-NOTFOUND)
   message(FATAL_ERROR Missing QMF schema processor.)
endif(QMFGEN-NOTFOUND)

# Figure out the qpid version
execute_process(COMMAND ${QPIDD} --version  OUTPUT_VARIABLE QPID_VERSION_LONG)
string(STRIP QPID_VERSION_LONG ${QPID_VERSION_LONG})
string(REGEX REPLACE "d \\(.*\\)" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})
string(REGEX REPLACE ".* " "" QPID_VERSION ${QPID_VERSION_LONG})
string(REGEX REPLACE "\\." "" QPID_VERSION ${QPID_VERSION})

message("Building against: ${QPID_VERSION_LONG}")
add_definitions(-DQPID_VERSION=${QPID_VERSION})

set(regen_schema OFF)
message(STATUS "Regenerating QMF schemas: ${SCHEMAS}")
foreach (schema_file ${SCHEMAS} ${QMFGEN})
    foreach (file ${HOST_SCHEMA_SOURCES} ${NET_SCHEMA_SOURCES})
        if (EXISTS ${file})
            if (${schema_file} IS_NEWER_THAN ${file})
                set(regen_schema ON)
	        message("${schema_file} is newer than ${file}")
            endif (${schema_file} IS_NEWER_THAN ${file})
        else (EXISTS ${file})
            set(regen_schema ON)
        endif (EXISTS ${file})
    endforeach (file ${HOST_SCHEMA_SOURCES} ${NET_SCHEMA_SOURCES})
endforeach (schema_file ${SCHEMAS})

if (regen_schema)
    # Regenerate everything 
    execute_process(COMMAND rm -f ${HOST_SCHEMA_SOURCES})
    execute_process(COMMAND ${QMFGEN} -o ./qmf ${SCHEMAS}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else (regen_schema)
    message(STATUS "No need to generate QMF schema")
endif (regen_schema)

# Add QF libraries
target_link_libraries(${M_HOST} ${QPIDCOMMON} ${QPIDCLIENT} ${QMF})
target_link_libraries(${M_NET}  ${QPIDCOMMON} ${QPIDCLIENT} ${QMF})

# Install targets
install(TARGETS ${M_HOST}  DESTINATION sbin)
install(TARGETS ${M_NET}   DESTINATION sbin)

install(FILES ${SCHEMAS} DESTINATION share/matahari)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari.h DESTINATION include)


