cmake_minimum_required(VERSION 2.6)
project (MATAHARI)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(MINGW_ROOT "/usr/i686-pc-mingw32/sys-root/mingw")

include(FindPkgConfig)
include_directories("${CMAKE_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/qmf")

set(PCRE_PATH "D:/Users/mcpierce/Programming/pcre-7.0")
set(QPID_PATH "C:/Program Files/Apache/qpid-0.6")

# set service name var's
set(M_HOST "matahari-hostd")
set(M_NET "matahari-netd")
set(M_PKG "matahari-packaged")
set(M_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

file(GLOB SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/*.xml")
install(FILES ${SCHEMAS} DESTINATION share/matahari)

# Main function plus QMF models
set(HOST_SOURCES
        matahari-host.cpp
        qmf/hostagent.cpp
        qmf/processoragent.cpp)

# TODO: host.cpp and processor.cpp need to be re-written in C
#       and then combined with util.c into a library/dll
#
#       Refer to qpid CMake control file for how to do this
set(HOST_LIBRARY
        host.cpp
        processor.cpp
        util.c)

set(HOST_SCHEMA_SOURCES
        ${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/host/EventHeartbeat.cpp
        ${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/host/Host.cpp
        ${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/host/Package.cpp
        ${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/host/Processor.cpp)

add_executable(${M_HOST} ${HOST_SOURCES} ${HOST_LIBRARY} ${HOST_SCHEMA_SOURCES})

set(NET_SOURCES
	matahari-netd.cpp
	qmf/netagent.cpp)

set(NET_SCHEMA_SOURCES
	${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/net/Package.cpp
	${CMAKE_BINARY_DIR}/qmf/com/redhat/matahari/net/Network.cpp)

add_executable(${M_NET} ${NET_SOURCES} ${NET_SCHEMA_SOURCES})

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

target_link_libraries(${M_HOST} ${QPIDCOMMON} ${QPIDCLIENT} ${QMF})
target_link_libraries(${M_NET} ${QPIDCOMMON} ${QPIDCLIENT} ${QMF})

execute_process(COMMAND ${QPIDD} --version  OUTPUT_VARIABLE QPID_VERSION_LONG)
string(STRIP QPID_VERSION_LONG ${QPID_VERSION_LONG})
string(REGEX REPLACE "d \\(.*\\)" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})
message("Building against: ${QPID_VERSION_LONG}")

string(REGEX REPLACE ".* " "" QPID_VERSION ${QPID_VERSION_LONG})
string(REGEX REPLACE "\\." "" QPID_VERSION ${QPID_VERSION})
add_definitions(-DQPID_VERSION=${QPID_VERSION})

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

find_program(QMFGEN qmf-gen)
if(QMFGEN-NOTFOUND)
   message(FATAL_ERROR Missing QMF schema processor.)
endif(QMFGEN-NOTFOUND)

set(regen_schema OFF)
message(STATUS "Regenerating QMF schemas: ${SCHEMAS}")
foreach (schema_file ${SCHEMAS} ${QMFGEN})
    foreach (file ${HOST_SCHEMA_SOURCES} ${NET_SCHEMA_SOURCES})
        if (${schema_file} IS_NEWER_THAN ${file})
            set(regen_schema ON)
	    message("${schema_file} is newer than ${file}")
        endif (${schema_file} IS_NEWER_THAN ${file})
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

set (CPACK_GENERATOR "NSIS")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../COPYING")

set (CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/windows/matahari-icon.ico")
set (CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/windows/matahari-icon.ico")
set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/windows/matahari-install-banner.bmp")
set (CPACK_NSIS_URL_INFO_ABOUT "http://fedorahosted.org/matahari")
# Needs this to correctly set up Start menu links later.
set (CPACK_PACKAGE_EXECUTABLES "")

# Cant supply arguments within quotes, use batch files to support --port etc
set (CPACK_NSIS_MENU_LINKS "")

set(CPACK_PACKAGE_NAME "matahari")
set(CPACK_PACKAGE_VENDOR "Red Hat")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Matahari QMF Agent")
set(CPACK_PACKAGE_VERSION "${M_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "matahari-${CPACK_PACKAGE_VERSION}")

set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "")
set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS} "ExecWait '\\\"$INSTDIR\\\\sbin\\\\install.bat\\\"' ")

set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "ExecWait '\\\"sc delete mh_host\\\"' ")
set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "ExecWait '\\\"sc delete mh_broker\\\"' ")

#WriteRegStr HKCU \\\"Environment\\\" \\\"OGITOR_HOME\\\" $INSTDIR
#configure_file(autorun.in arun )

install(TARGETS ${M_HOST}  DESTINATION sbin)
install(TARGETS ${M_NET}   DESTINATION sbin)

if(WIN32)
  # Find qpid files needed by matahari
  file(GLOB_RECURSE QPID_EXE  ${MINGW_ROOT}/*/qpidd.exe)
  file(GLOB_RECURSE QMF_DLL   ${MINGW_ROOT}/*/libqmf*.dll)
  file(GLOB_RECURSE QPID_DLL  ${MINGW_ROOT}/*/libqpid*.dll)
  install(FILES ${QPID_EXE} ${QMF_DLL} ${QPID_DLL} DESTINATION sbin)

  # Find additional libraries needed by matahari/qpid
  file(GLOB_RECURSE GCC_DLL   ${MINGW_ROOT}/*/libgcc_*.dll)
  file(GLOB_RECURSE PCRE_DLL  ${MINGW_ROOT}/*/libpcre*.dll)
  file(GLOB_RECURSE VIRT_DLL  ${MINGW_ROOT}/*/libvirt*.dll)
  file(GLOB_RECURSE BOOST_DLL ${MINGW_ROOT}/*/boost_*-mt-1*.dll)
  install(FILES ${GCC_DLL} ${PCRE_DLL} ${VIRT_DLL} ${BOOST_DLL} DESTINATION sbin)

  # Util for creating services
  file(GLOB_RECURSE SRVANY_EXE ${MINGW_ROOT}/*/rhsrvany.exe)
  install(FILES ${SRVANY_EXE} DESTINATION sbin)
  install(FILES windows/install.bat DESTINATION sbin)

endif(WIN32)

INCLUDE(InstallRequiredSystemLibraries)


INCLUDE(CPack)
