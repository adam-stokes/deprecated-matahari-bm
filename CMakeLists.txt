cmake_minimum_required(VERSION 2.6)
project (MATAHARI)
set (PROJECT_VERSION "0.0.5")

# install docs
install(FILES COPYING AUTHORS DESTINATION doc/matahari-${PROJECT_VERSION})

# add sub-directories
add_subdirectory(src)

# install schema files
file(GLOB SCHEMA "src/*.xml")
install(FILES ${SCHEMA} DESTINATION share/matahari)

# check includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (sys/ioctl.h HAVE_SYS_IOCTL_H)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)
