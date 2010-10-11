cmake_minimum_required(VERSION 2.6)

project (MATAHARI)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(PROJECT_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
message("top: ${CMAKE_BINARY_DIR} - ${CMAKE_CURRENT_BINARY_DIR} - ${PROJECT_BINARY_DIR}")

# Various defines 
set(MINGW_ROOT "/usr/i686-pc-mingw32/sys-root/mingw")
set(PCRE_PATH "D:/Users/mcpierce/Programming/pcre-7.0")
set(QPID_PATH "C:/Program Files/Apache/qpid-0.6")

# add sub-directories
add_subdirectory(src)

# Set some prefixes
set(localstatedir "/var")
set(sysconfdir    "/etc")

set(CPACK_PACKAGE_NAME "matahari")
set(CPACK_PACKAGE_VENDOR "Red Hat")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Matahari QMF Agent")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "matahari-${CPACK_PACKAGE_VERSION}")

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
  
  set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "")
  set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS} "ExecWait '\\\"$INSTDIR\\\\sbin\\\\install.bat\\\"' ")
  
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "")
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "ExecWait '\\\"sc delete mh_host\\\"' ")
  set( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS} "ExecWait '\\\"sc delete mh_broker\\\"' ")
  
  #WriteRegStr HKCU \\\"Environment\\\" \\\"OGITOR_HOME\\\" $INSTDIR
  #configure_file(autorun.in arun )
  
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

  # Windows configs
  install(FILES matahari-broker.conf DESTINATION /etc)

else(WIN32)

  set (CPACK_GENERATOR "TGZ")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.init.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari.init)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.init.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.sysconf.in
		 ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf)

endif(WIN32)

INCLUDE(InstallRequiredSystemLibraries)
INCLUDE(CPack)
