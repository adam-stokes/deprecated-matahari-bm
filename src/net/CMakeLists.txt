set(NET_LIB "mnet")
set(NET_QMF "matahari-qmf-netd")
set(NET_DBUS "matahari-dbus-netd")

# QMF daemon
if(WITH-QMF)
    add_executable(${NET_QMF} matahari-net.cpp ${SCHEMA_SOURCES} ../lib/mh_agent.cpp)
    target_link_libraries(${NET_QMF} ${NET_LIB} mcommon)

    target_link_libraries(${NET_QMF} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(${NET_QMF} ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${NET_QMF} wsock32)
    endif(WIN32)

    install(TARGETS ${NET_QMF} DESTINATION sbin)
endif(WITH-QMF)


# DBus daemon
if(WITH-DBUS)
    # Auto-generated stuff
    include(MatahariDBusMacros)
    generate_dbus_headers(network ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Network.xml)
    # Must be included to find matahari-API-dbus-properties.h
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(NET_DBUS_SOURCE
        matahari-network-dbus.c
        matahari-network-dbus-glue.h
        matahari-network-dbus-properties.h
    )

    add_executable(${NET_DBUS} ${NET_DBUS_SOURCE})

    target_link_libraries(${NET_DBUS} ${NET_LIB} mcommon_dbus)

    include_directories(${polkit_INCLUDE_DIRS})
    target_link_libraries(${NET_DBUS} ${polkit_LIBRARIES})

    include_directories(${dbus-glib_INCLUDE_DIRS})
    target_link_libraries(${NET_DBUS} ${dbus-glib_LIBRARIES})

    # Install targets
    # TODO: fix hardcoded paths, should go to libexec
    install(TARGETS ${NET_DBUS} DESTINATION sbin)
    install(FILES ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Network.xml DESTINATION share/dbus-1/interfaces)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Network.service DESTINATION share/dbus-1/system-services)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Network.conf DESTINATION /etc/dbus-1/system.d)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Network.policy DESTINATION share/polkit-1/actions)
endif(WITH-DBUS)
