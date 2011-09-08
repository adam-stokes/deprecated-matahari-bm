cmake_minimum_required(VERSION 2.6)

project (MATAHARI)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "4")
set(PROJECT_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

MACRO (inherit_value theSymbol theValue)
    if (NOT DEFINED ${theSymbol})
        set (${theSymbol} ${theValue})
        # message ("Set symbol '${theSymbol}' to value '${theValue}'")
        set (${theSymbol}_inherited = "true")
    endif (NOT DEFINED ${theSymbol})
ENDMACRO (inherit_value)

# Various defines 
inherit_value(MATAHARI_PORT "49000")
inherit_value(MATAHARI_BROKER "127.0.0.1")
inherit_value(SUPPORT_TRACING 0)

# Build Paths
inherit_value(MINGW_ROOT "/usr/i686-pc-mingw32/sys-root/mingw")
inherit_value(PCRE_PATH "D:/Users/mcpierce/Programming/pcre-7.0")
inherit_value(QPID_PATH "C:/Program Files/Apache/qpid-0.6")

# Build Prefixes
inherit_value(localstatedir "/var")
inherit_value(sysconfdir    "${SYSCONF_INSTALL_DIR}")
inherit_value(initdir	    "${SYSCONF_INSTALL_DIR}/init.d")

# Add sub-directories
add_subdirectory(src)

# Installer
set(CPACK_PACKAGE_NAME "matahari")
set(CPACK_PACKAGE_VENDOR "Red Hat")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Matahari QMF Agent")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "matahari-${CPACK_PACKAGE_VERSION}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.sysconf.in
	       ${CMAKE_CURRENT_BINARY_DIR}/matahari.sysconf)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.conf.in
	       ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.conf)

if(WIN32)

  set (CPACK_GENERATOR "NSIS")
  set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  
  set (CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/matahari-icon.ico")
  set (CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/matahari-icon.ico")
  set (CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/matahari-install-banner.bmp")
  set (CPACK_NSIS_URL_INFO_ABOUT "http://fedorahosted.org/matahari")

  # Needs this to correctly set up Start menu links later.
  set (CPACK_PACKAGE_EXECUTABLES "")
  
  # Cant supply arguments within quotes, use batch files to support --port etc
  set (CPACK_NSIS_MENU_LINKS "")  

  # The comments and spacing are apparnetly critical to all this working, don't ask me why
  set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "\\n")
  set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
					 " Some comment\\n  ReadRegStr $1 HKLM SYSTEM\\\\CurrentControlSet\\\\services\\\\Matahari Broker\\n"
					 " Some comment\\n  DetailPrint \\\"Current broker is: $1\\\"\\n"
					 " Some comment\\n  StrCmp $1 \\\"\\\" 0 +2\\n"
					 " Some comment\\n  WriteRegStr HKLM SYSTEM\\\\CurrentControlSet\\\\services\\\\Matahari Broker 127.0.0.1\\n"
					 " Some comment\\n  ExecWait \\\"$INSTDIR\\\\sbin\\\\install.bat\\\"\\n"
					 " Some comment\\n  Sleep 100\\n"
					 )
  
  # The comments and spacing are apparnetly critical to all this working, don't ask me why
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "\\n")
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} 
  					   " Some comment\\n  DetailPrint \\\"Stopping Matahari services\\\"\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe stop mh_hostd\\\"'\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe stop mh_netd\\\"'\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe stop mh_serviced\\\"'\\n"
					   " Give them time to stop\\n  Sleep 10000\\n"
  					   " Some comment\\n  DetailPrint \\\"Removing Matahari services\\\"\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe delete mh_hostd\\\"'\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe delete mh_netd\\\"'\\n"
					   " Some comment\\n  ExecWait '\\\"$SYSDIR\\\\sc.exe delete mh_serviced\\\"'\\n"
  					   " Some comment\\n  DetailPrint \\\"Removing Matahari service binaries\\\"\\n"
					   " Some comment\\n  ExecWait 'del \\\"$INSTDIR\\\\mh_hostd.exe\\\"'\\n"
					   " Some comment\\n  ExecWait 'del \\\"$INSTDIR\\\\mh_netd.exe\\\"'\\n"
					   " Some comment\\n  ExecWait 'del \\\"$INSTDIR\\\\mh_serviced.exe\\\"'\\n"
					   )

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/windows/autorun.inf.in 
                 ${CMAKE_CURRENT_BINARY_DIR}/src/windows/autorun.inf)
  
  # Assume all installed DLLs are needed by matahari/qpid
  # Finding the minimal list is excruciatingly painful and unreliable
  file(GLOB_RECURSE DLL_LIST ${MINGW_ROOT}/*/*.dll)
  install(FILES ${DLL_LIST} DESTINATION sbin)

  # Util for creating services
  file(GLOB_RECURSE SRVANY_EXE ${MINGW_ROOT}/*/srvany.exe)
  install(FILES ${SRVANY_EXE} DESTINATION sbin)
  install(FILES src/windows/install.bat DESTINATION sbin)

else(WIN32)

  set (CPACK_GENERATOR "TGZ")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.init.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari.init)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.init.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker @ONLY)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.sysconf.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf @ONLY)

  install(DIRECTORY DESTINATION ${localstatedir}/lib/matahari
          DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                                GROUP_READ GROUP_WRITE GROUP_EXECUTE)
  install(CODE "execute_process(COMMAND chown root:qpidd ${localstatedir}/lib/matahari)")

  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker DESTINATION ${initdir})

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari.sysconf DESTINATION ${sysconfdir}/sysconfig RENAME matahari)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf DESTINATION ${sysconfdir}/sysconfig RENAME matahari-broker)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.conf DESTINATION ${sysconfdir})

  # Check for systemd
  include(FindPkgConfig)
  pkg_check_modules(systemd systemd)

  if(systemd_FOUND)
    set(systemdunitdir "/lib/systemd/system")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.service.in
                   ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.service)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.service DESTINATION ${systemdunitdir})
  endif(systemd_FOUND)

endif(WIN32)

INCLUDE(InstallRequiredSystemLibraries)
INCLUDE(CPack)
