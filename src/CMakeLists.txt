cmake_minimum_required(VERSION 2.6)
project (MATAHARI)

# set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/qmf")

set(SCHEMAS
	schema-host.xml)

set(MAIN_SOURCES
	host.cpp
	main.cpp
	processor.cpp
	util.c
	qmf/hostagent.cpp
	qmf/processoragent.cpp)

set(GENERATED_SOURCES
	qmf/com/redhat/matahari/host/EventHeartbeat.cpp
	qmf/com/redhat/matahari/host/EventHeartbeat.h
	qmf/com/redhat/matahari/host/Host.cpp
	qmf/com/redhat/matahari/host/Host.h
	qmf/com/redhat/matahari/host/Package.cpp
	qmf/com/redhat/matahari/host/Package.h
	qmf/com/redhat/matahari/host/Processor.cpp
	qmf/com/redhat/matahari/host/Processor.h)

IF(WIN32)
	set(PCRE_PATH "D:/Users/mcpierce/Programming/pcre-7.0")
	set(QPID_PATH "C:/Program Files/Apache/qpid-0.6")

	find_path(PCRE_INCLUDE pcre.h "${PCRE_PATH}/include")

	if(PCRE_INCLUDE)
		include_directories(${PCRE_INCLUDE})
	else(PCRE_INCLUDE)
		message(FATAL_ERROR "PCRE include files not found.")
	endif(PCRE_INCLUDE)

	#find_path(QPID_INCLUDE NAMES qpid/Address.h "${QPID_PATH}/include")

	#if(QPID_INCLUDE)
	#	include_directories(${QPID_INCLUDE})
	#else(QPID_INCLUDE)
	#	message(FATAL_ERROR "Qpid include files not found.")
	#endif(QPID_INCLUDE)
ENDIF(WIN32)

add_executable(matahari ${MAIN_SOURCES} ${GENERATED_SOURCES})

IF(WIN32)
	find_library(PCRE_LIBRARY NAMES pcre.lib PATHS ${PCRE_PATH}/lib)

	if(PCRE_LIBRARY)
		target_link_libraries(matahari "${PCRE_LIBRARY}")
	else(PCRE_LIBRARY)
		message(FATAL_ERROR "PCRE library file not found.")
	endif(PCRE_LIBRARY)

	find_library(QMFCONSOLE_LIBRARY
		NAME qmfconsole.lib
		PATHS ${QPID_PATH}/bin)
	find_library(QPIDCLIENT_LIBRARY
		NAME qpidclient.lib
		PATHS ${QPID_PATH}/bin)
	find_library(QPIDCOMMON_LIBRARY
		NAME qpidcommon.lib
		PATHS ${QPID_PATH}/bin)

	if(QMFCONSOLE_LIBRARY AND QPIDCLIENT_LIBRARY AND QPIDCOMMON_LIBRARY)
		target_link_libraries(matahari "${QMFCONSOLE}")
		target_link_libraries(matahari "${QPIDCLIENT}")
		target_link_libraries(matahari "${QPIDCOMMON}")
	else(QMFCONSOLE_LIBRARY AND QPIDCLIENT_LIBRARY AND QPIDCOMMON_LIBRARY)
		message(FATAL_ERROR "Missing Qpid libraries")
	endif(QMFCONSOLE_LIBRARY AND QPIDCLIENT_LIBRARY AND QPIDCOMMON_LIBRARY)
ELSE(WIN32)
	find_library(PCRE pcre)
	target_link_libraries(matahari ${PCRE})

	find_library(QPIDCLIENT qpidclient)
	find_library(QPIDCOMMON  qpidcommon)
	target_link_libraries(matahari ${QPIDCLIENT} ${QPIDCOMMON})

	find_library(QMF qmf)
	target_link_libraries(matahari ${QMF})

	find_library(VIRT virt)
	target_link_libraries(matahari ${VIRT})
ENDIF(WIN32)
