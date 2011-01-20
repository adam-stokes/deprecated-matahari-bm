set(MHOST_VERSION 0.0.1)

include(DBusMacros)
generate_dbus_headers(host)

set(HOST_DBUS_SOURCE
    matahari-host-dbus.c
    ../common/mh_dbus_common.c
    ${CMAKE_CURRENT_BINARY_DIR}/matahari-host-dbus-glue.h
    ${CMAKE_CURRENT_BINARY_DIR}/matahari-host-dbus-properties.h
)

# Daemon
set(M_HOST_DBUS "matahari-host-dbusd")
add_executable(${M_HOST_DBUS} ${HOST_DBUS_SOURCE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${M_HOST_DBUS} mhost )

if(polkit_FOUND)
   include_directories(${polkit_INCLUDE_DIRS})
   target_link_libraries(${M_HOST_DBUS} ${polkit_LIBRARIES})
else(polkit_FOUND)
   message(FATAL_ERROR "polkit headers/libraries not found.")
endif(polkit_FOUND)

if(dbus-glib_FOUND)
   include_directories(${dbus-glib_INCLUDE_DIRS})
   target_link_libraries(${M_HOST_DBUS} ${dbus-glib_LIBRARIES})
else(dbus-glib_FOUND)
   message(FATAL_ERROR "dbus-glib headers/libraries not found.")
endif(dbus-glib_FOUND)

# Install targets
# TODO: fix hardcoded paths, should go to libexec
install(TARGETS ${M_HOST_DBUS} DESTINATION sbin)
install(FILES ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.host.xml DESTINATION share/dbus-1/interfaces)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.host.service DESTINATION share/dbus-1/system-services)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.host.conf DESTINATION etc/dbus-1/system.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.host.policy DESTINATION share/polkit-1/actions)
