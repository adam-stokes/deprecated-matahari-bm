set(MHOST_VERSION 0.0.1)

include(DBusMacros)
generate_dbus_headers(Services)

set(SRV_DBUS_SOURCE
    matahari-services-dbus.c
    ../common/mh_dbus_common.c
    ${CMAKE_CURRENT_BINARY_DIR}/matahari-services-dbus-glue.h
    ${CMAKE_CURRENT_BINARY_DIR}/matahari-services-dbus-properties.h
)

# Daemon
set(M_SRV_DBUS "matahari-services-dbusd")
add_executable(${M_SRV_DBUS} ${SRV_DBUS_SOURCE})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${M_SRV_DBUS} msrv)

if(polkit_FOUND)
   include_directories(${polkit_INCLUDE_DIRS})
   target_link_libraries(${M_SRV_DBUS} ${polkit_LIBRARIES})
else(polkit_FOUND)
   message(FATAL_ERROR "polkit headers/libraries not found.")
endif(polkit_FOUND)

if(dbus-glib_FOUND)
   include_directories(${dbus-glib_INCLUDE_DIRS})
   target_link_libraries(${M_SRV_DBUS} ${dbus-glib_LIBRARIES})
else(dbus-glib_FOUND)
   message(FATAL_ERROR "dbus-glib headers/libraries not found.")
endif(dbus-glib_FOUND)

# Install targets
# TODO: fix hardcoded paths, should go to libexec
install(TARGETS ${M_SRV_DBUS} DESTINATION sbin)
install(FILES ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Services.xml DESTINATION share/dbus-1/interfaces)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.service DESTINATION share/dbus-1/system-services)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.conf DESTINATION /etc/dbus-1/system.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.policy DESTINATION share/polkit-1/actions)
