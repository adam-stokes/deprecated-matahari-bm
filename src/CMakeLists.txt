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
   option(WITH-DBUS "Build the DBus daemon" ON)
endif(WIN32)
option(WITH-QMF "Build the QMF daemon" ON)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")

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
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/matahari.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

## Modules
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

## dbus
if(WITH-DBUS)
    pkg_check_modules(dbus-glib REQUIRED dbus-glib-1)
    pkg_check_modules(polkit REQUIRED polkit-gobject-1)
    
    if(NOT dbus-glib_FOUND)
        message(FATAL_ERROR "dbus-glib headers/libraries not found.")
    endif(NOT dbus-glib_FOUND)

    if(NOT polkit_FOUND)
        message(FATAL_ERROR "polkit headers/libraries not found.")
    endif(NOT polkit_FOUND)

    # Create src/dbus directory and generate DBus XML definition files into it
    execute_process(COMMAND mkdir -p src/dbus)
    find_file(XSLTPROC xsltproc)
    execute_process(
        COMMAND ${XSLTPROC} ${CMAKE_CURRENT_SOURCE_DIR}/schema-to-dbus.xsl ${CMAKE_CURRENT_SOURCE_DIR}/schema.xml
        WORKING_DIRECTORY src/dbus
    )
endif(WITH-DBUS)

## qpid
if(WITH-QMF)
    # Defines
    add_definitions(-DSUPPORT_TRACING=${SUPPORT_TRACING})
    add_definitions(-DMATAHARI_PORT=${MATAHARI_PORT})
    add_definitions(-DMATAHARI_BROKER="${MATAHARI_BROKER}")

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


    find_package(QPID REQUIRED)
    if(QPID-NOTFOUND)
        message(FATAL_ERROR "QPID library not found.")
    endif(QPID-NOTFOUND)

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
endif(WITH-QMF)


### Subdirectories
add_subdirectory(lib)
add_subdirectory(host)
add_subdirectory(service)
add_subdirectory(net)


### Installation
install(FILES ${SCHEMAS} DESTINATION share/matahari)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/matahari.h DESTINATION include)
install(FILES cmake/modules/FindMatahari.cmake DESTINATION share/cmake/Modules)

if(WITH-QMF)
    install(FILES cmake/modules/FindQPID.cmake DESTINATION share/cmake/Modules)
    install(FILES include/matahari/mh_agent.h DESTINATION include/matahari)
endif(WITH-QMF)

if(WITH-DBUS)
    install(FILES schema-to-dbus.xsl DESTINATION share/matahari)
    install(FILES include/matahari/mh_dbus_common.h DESTINATION include/matahari)
    install(FILES include/matahari/mh_gobject_class.h DESTINATION include/matahari)
    install(FILES cmake/modules/MatahariDBusMacros.cmake DESTINATION share/cmake/Modules)
endif(WITH-DBUS)
