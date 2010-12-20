set(MHOST_VERSION 0.0.1)

# set service name var's
set(M_HOST_DBUS "matahari-host-dbusd")
set(M_HOST_QMF "matahari-host-qmfd")

set(DBUS_INTROSPECT "${CMAKE_CURRENT_SOURCE_DIR}/matahari-host-dbus.xml")

set(HOST_DBUS_SOURCE "matahari-host-dbus.c")
set(HOST_QMF_SOURCE "matahari-host-qmf.cpp")

# Daemon
add_executable(${M_HOST_DBUS} ${HOST_DBUS_SOURCE})
target_link_libraries(${M_HOST_DBUS} mhost)

#add_executable(${M_HOST_QMF} ${HOST_QMF_SOURCE})
#target_link_libraries(${M_HOST_QMF} mhost)

# DBus
find_file(BIND_TOOL dbus-binding-tool)
execute_process(COMMAND ${BIND_TOOL}
                --prefix=matahari --mode=glib-server
                --output=matahari-host-dbus-glue.h ${DBUS_INTROSPECT}
#                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/fmci})
)

# Install targets
# TODO: fix hardcoded paths, should go to libexec
install(TARGETS ${M_HOST_DBUS} DESTINATION sbin)
#install(TARGETS ${M_HOST_QMF} DESTINATION sbin)
install(FILES ${DBUS_INTROSPECT} DESTINATION share/dbus-1/interfaces)
install(FILES org.matahariproject.Host.conf etc/dbus-1/system.d)