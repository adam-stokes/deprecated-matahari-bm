set(BASE "host")
set(BASE_LIB "m${BASE}")
set(QMF_AGENT "matahari-qmf-${BASE}d")
set(DBUS_AGENT "matahari-dbus-${BASE}d")

# QMF daemon
if(WITH-QMF)
    add_executable(${QMF_AGENT} matahari-host.cpp ${SCHEMA_SOURCES})
    target_link_libraries(${QMF_AGENT} ${BASE_LIB} mcommon mcommon_qmf)

    target_link_libraries(${QMF_AGENT} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(${QMF_AGENT} ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${QMF_AGENT} wsock32)
    endif(WIN32)

    install(TARGETS ${QMF_AGENT} DESTINATION sbin)
endif(WITH-QMF)


# DBus daemon
if(WITH-DBUS)
    # Auto-generated stuff
    include(MatahariDBusMacros)
    generate_dbus_headers(host ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Host.xml)
    # Must be included to find matahari-API-dbus-properties.h
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(DBUS_AGENT_SOURCE
        matahari-host-dbus.c
        matahari-host-dbus-glue.h
        matahari-host-dbus-properties.h
    )

    add_executable(${DBUS_AGENT} ${DBUS_AGENT_SOURCE})

    target_link_libraries(${DBUS_AGENT} ${BASE_LIB} mcommon_dbus ${dbus-glib_LIBRARIES})
    include_directories(${dbus-glib_INCLUDE_DIRS})

    # Install targets
    # TODO: fix hardcoded paths, should go to libexec
    install(TARGETS ${DBUS_AGENT} DESTINATION sbin)
    install(FILES ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Host.xml DESTINATION share/dbus-1/interfaces)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.service DESTINATION share/dbus-1/system-services)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.conf DESTINATION /etc/dbus-1/system.d)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.policy DESTINATION share/polkit-1/actions)
endif(WITH-DBUS)
