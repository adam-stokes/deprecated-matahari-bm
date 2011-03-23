# - Try to find the QPID

if(MSVC)
   find_file(QPIDD qpidd.exe "${QPID_PATH}")
   find_path(QPID_INCLUDE Address.h "${QPID_PATH}")
   find_library(QMFCONSOLE_LIBRARY qmfconsole.lib "${QPID_PATH}")
   find_library(QPIDCLIENT_LIBRARY qpidclient.lib "${QPID_PATH}")
   find_library(QPIDCOMMON_LIBRARY qpidcommon.lib "${QPID_PATH}")
   find_library(QPIDCOMMON_LIBRARY qpidtypes.lib "${QPID_PATH}")

   # Figure out some way to do this automatically on windows
   # Perhaps with the strings command
   set(QPID_VERSION_LONG "qpidd (qpidc) version 0.6")

else(MSVC)
   find_library(QMF_LIBRARY qmf)
   find_library(QMF2_LIBRARY qmf2)
   find_library(QPIDCLIENT_LIBRARY qpidclient)
   find_library(QPIDCOMMON_LIBRARY qpidcommon)
   find_library(QPIDTYPES_LIBRARY qpidtypes)
   find_library(QPIDMESSAGING_LIBRARY qpidmessaging)

   # Check if we found all libraries
   if(QMF_LIBRARY)
       set(QMF_FOUND true)
   endif(QMF_LIBRARY)

   if(QMF2_LIBRARY)
       set(QMF2_FOUND true)
   endif(QMF2_LIBRARY)

   if(QPIDCLIENT_LIBRARY)
       set(QPIDCLIENT_FOUND true)
   endif(QPIDCLIENT_LIBRARY)

   if(QPIDCOMMON_LIBRARY)
       set(QPIDCOMMON_FOUND true)
   endif(QPIDCOMMON_LIBRARY)

   if(QPIDTYPES_LIBRARY)
       set(QPIDTYPES_FOUND true)
   endif(QPIDTYPES_LIBRARY)

   if(QPIDMESSAGING_LIBRARY)
       set(QPIDMESSAGING_FOUND true)
   endif(QPIDMESSAGING_LIBRARY)

   # Handle found/not found libraries
   if(QMF_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QMF: ${QMF_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QMF_FOUND)
       if(QPID_FIND_REQUIRED)
           message(FATAL_ERROR "Could not find QMF")
       endif(QPID_FIND_REQUIRED)
   endif(QMF_FOUND)

   if(QMF2_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QMF2: ${QMF2_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QMF2_FOUND)
       if(QPID_FIND_REQUIRED)
           message(FATAL_ERROR "Could not find QMF2")
       endif(QPID_FIND_REQUIRED)
   endif(QMF2_FOUND)

   if(QPIDCLIENT_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QPIDCLIENT: ${QPIDCLIENT_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QPIDCLIENT_FOUND)
       if(QPID_FIND_REQUIRED)
           message(FATAL_ERROR "Could not find QPIDCLIENT")
       endif(QPID_FIND_REQUIRED)
   endif(QPIDCLIENT_FOUND)

   if(QPIDCOMMON_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QPIDCOMMON: ${QPIDCOMMON_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QPIDCOMMON_FOUND)
       if(QPID_FIND_REQUIRED)
           message(FATAL_ERROR "Could not find QPIDCOMMON")
       endif(QPID_FIND_REQUIRED)
   endif(QPIDCOMMON_FOUND)

   if(QPIDTYPES_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QPIDTYPES: ${QPIDTYPES_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QPIDTYPES_FOUND)
       if(QPID_FIND_REQUIRED)
           # we don't want to fail now even we don't have QPIDTYPES!
           # TODO: fix once QPIDTYPES available
           message(STATUS "Could not find QPIDTYPES")
       endif(QPID_FIND_REQUIRED)
   endif(QPIDTYPES_FOUND)

   if(QPIDMESSAGING_FOUND)
       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QPIDMESSAGING: ${QPIDMESSAGING_LIBRARY}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QPIDMESSAGING_FOUND)
       if(QPID_FIND_REQUIRED)
           # we don't want to fail now even we don't have QPIDMESSAGING!
           # TODO: fix once QPIDMESSAGING available
           message(STATUS "Could not find QPIDMESSAGING")
       endif(QPID_FIND_REQUIRED)
   endif(QPIDMESSAGING_FOUND)

   # Figure out the qpid version
   if(WIN32)
      find_file(QPIDD qpidd.exe)
      find_file(QMFGEN qmf-gen PATHS "/usr/i686-pc-mingw32/sys-root/mingw/managementgen")
      if(QPIDD)
          execute_process(COMMAND strings ${QPIDD} OUTPUT_VARIABLE QPID_VERSION_LONG)
          string(REGEX REPLACE ".*BUILD/qpid" " qpid" QPID_VERSION_LONG ${QPID_VERSION_LONG})
          string(REGEX REPLACE "/cpp.*" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})

          #version: qpid-cpp-0.6.895736
          string(REGEX REPLACE "qpid-cpp-" "" QPID_VERSION ${QPID_VERSION_LONG})
          string(REGEX REPLACE ".[0-9]*$" "" QPID_VERSION ${QPID_VERSION})
      else(QPIDD)
          if(QPID_FIND_REQUIRED)
              message(FATAL_ERROR "Could not determine Qpid version, probably QPIDD not installed")
          endif(QPID_FIND_REQUIRED)
      endif(QPIDD)

   else(WIN32)
      find_file(QPIDD qpidd)
      set(QPIDD false)
      find_program(QMFGEN qmf-gen)
      if(QPIDD)
          execute_process(COMMAND ${QPIDD} --version  OUTPUT_VARIABLE QPID_VERSION_LONG)
          string(STRIP QPID_VERSION_LONG ${QPID_VERSION_LONG})
          string(REGEX REPLACE "d \\(.*\\)" "" QPID_VERSION_LONG ${QPID_VERSION_LONG})
          string(REGEX REPLACE ".* " "" QPID_VERSION ${QPID_VERSION_LONG})
      else(QPIDD)
	  set(QPID_FIND_REQUIRED false)
          if(QPID_FIND_REQUIRED)
              message(FATAL_ERROR "Could not determine Qpid version, probably QPIDD not installed")
          endif(QPID_FIND_REQUIRED)
      endif(QPIDD)
   endif(WIN32)

   # Look for schema processor
   if(QMFGEN)
       set(QMFGEN_FOUND true)

       if(NOT QPID_FIND_QUIETLY)
           message(STATUS "Found QMF schema processor: ${QMFGEN}")
       endif(NOT QPID_FIND_QUIETLY)
   else(QMFGEN)
       if(QPID_FIND_REQUIRED)
           message(FATAL_ERROR "Could not find QMF schema processor")
       endif(QPID_FIND_REQUIRED)
   endif(QMFGEN)

endif(MSVC)
