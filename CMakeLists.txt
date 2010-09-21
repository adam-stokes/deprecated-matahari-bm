cmake_minimum_required(VERSION 2.6)
project (MATAHARI)
set (PROJECT_VERSION "0.4.0")

# add sub-directories
add_subdirectory(src)

# check includes
include (CheckIncludeFiles)
check_include_files (string.h HAVE_STRING_H)
check_include_files (sys/ioctl.h HAVE_SYS_IOCTL_H)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/config.h)

# Set some prefixes
set(localstatedir "/var")
set(sysconfdir "/etc")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.init.in
	${CMAKE_CURRENT_BINARY_DIR}/matahari.init)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.init.in
	${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.init)

