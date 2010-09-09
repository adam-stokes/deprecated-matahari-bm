cmake_minimum_required(VERSION 2.6)
project (MATAHARI)

set(CMAKE_VERBOSE_MAKEFILE ON)

include(FindPkgConfig)
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/qmf")

set(PCRE_PATH "D:/Users/mcpierce/Programming/pcre-7.0")
set(QPID_PATH "C:/Program Files/Apache/qpid-0.6")

# set service name var's
set(M_HOST "matahari-host")
set(M_NET "matahari-net")
set(M_PKG "matahari-package")

set(SCHEMAS
        schema-host.xml)

set(HOST_SOURCES
        host.cpp
        main.cpp
        processor.cpp
        util.c
        qmf/hostagent.cpp
        qmf/processoragent.cpp)

set(GENERATED_SCHEMA_SOURCES
        qmf/com/redhat/matahari/host/EventHeartbeat.cpp
        qmf/com/redhat/matahari/host/EventHeartbeat.h
        qmf/com/redhat/matahari/host/Host.cpp
        qmf/com/redhat/matahari/host/Host.h
        qmf/com/redhat/matahari/host/Package.cpp
        qmf/com/redhat/matahari/host/Package.h
        qmf/com/redhat/matahari/host/Processor.cpp
        qmf/com/redhat/matahari/host/Processor.h)

add_executable(${M_HOST} ${HOST_SOURCES} ${GENERATED_SCHEMA_SOURCES})

pkg_check_modules(pcre REQUIRED libpcre)

if(NOT PKG_CONFIG_FOUND)
   message("pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)

if(WIN32)
   target_link_libraries(${M_HOST} wsock32)
else(WIN32)
   find_library(VIRT virt)
   target_link_libraries(${M_HOST} ${VIRT})
endif(WIN32)

if(MSVC)
   find_path(QPID_INCLUDE Address.h "${QPID_PATH}")
   find_library(QMFCONSOLE qmfconsole.lib "${QPID_PATH}")
   find_library(QPIDCLIENT qpidclient.lib "${QPID_PATH}")
   find_library(QPIDCOMMON qpidcommon.lib "${QPID_PATH}")
else(MSVC)
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

set(simple_libs check libvirt udev_new)
foreach(lib ${simple_libs}) 
    find_library(${lib}_LIBRARIES ${lib})
    if(${lib}_LIBRARIES) 
        target_link_libraries(${M_HOST} ${${lib}_LIBRARIES}})
    else(${lib}_LIBRARIES) 
    	message("Library ${lib} not found")
    endif(${lib}_LIBRARIES) 
endforeach(lib ${simple_libs})

find_program(QMFGEN qmf-gen)
if(QMFGEN-NOTFOUND)
   message(FATAL_ERROR Missing QMF schema processor.)
endif(QMFGEN-NOTFOUND)

set(regen_schema OFF)
message(STATUS "Regenerating QMF schemas")
foreach (schema_file ${SCHEMAS} ${QMFGEN})
    foreach (file ${GENERATED_SCHEMA_SOURCES})
        if (${schema_file} IS_NEWER_THAN ${file})
            set(regen_schema ON)
	    message("${schema_file} is newer than ${file}")
        endif (${schema_file} IS_NEWER_THAN ${file})
    endforeach (file ${GENERATED_SCHEMA_SOURCES})
endforeach (schema_file ${SCHEMAS})

if (regen_schema)
    # Regenerate everything 
    execute_process(COMMAND rm -f ${GENERATED_SCHEMA_SOURCES})
    execute_process(COMMAND ${QMFGEN} -o ./qmf ${SCHEMAS}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else (regen_schema)
    message(STATUS "No need to generate QMF schema")
endif (regen_schema)

