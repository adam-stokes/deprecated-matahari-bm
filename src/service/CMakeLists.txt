set(SRV_LIB "msrv")
set(SRV_QMF "matahari-qmf-serviced")
set(SRV_DBUS "matahari-dbus-serviced")

# QMF daemon
if(WITH-QMF)
    add_executable(${SRV_QMF} matahari-srv.cpp ${SCHEMA_SOURCES})
    target_link_libraries(${SRV_QMF} ${SRV_LIB} mcommon mcommon_qmf)

    target_link_libraries(${SRV_QMF} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    IF(QPIDTYPES_LIBRARY)
        target_link_libraries(${SRV_QMF} ${QPIDTYPES_LIBRARY})
    ENDIF(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${daemon} wsock32)
    endif(WIN32)

    install(TARGETS ${SRV_QMF} DESTINATION sbin)
endif(WITH-QMF)


# DBus daemon
if(WITH-DBUS)
    # Auto-generated stuff
    include(MatahariDBusMacros)
    generate_dbus_headers(services ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Services.xml)
    # Must be included to find matahari-API-dbus-properties.h
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(SRV_DBUS_SOURCE
        matahari-services-dbus.c
        matahari-services-dbus-glue.h
        matahari-services-dbus-properties.h
    )

    add_executable(${SRV_DBUS} ${SRV_DBUS_SOURCE})

    target_link_libraries(${SRV_DBUS} ${SRV_LIB} mcommon_dbus)

    include_directories(${polkit_INCLUDE_DIRS})
    target_link_libraries(${SRV_DBUS} ${polkit_LIBRARIES})

    include_directories(${dbus-glib_INCLUDE_DIRS})
    target_link_libraries(${SRV_DBUS} ${dbus-glib_LIBRARIES})

    # Install targets
    # TODO: fix hardcoded paths, should go to libexec
    install(TARGETS ${SRV_DBUS} DESTINATION sbin)
    install(FILES ${CMAKE_BINARY_DIR}/src/dbus/org.matahariproject.Services.xml DESTINATION share/dbus-1/interfaces)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.service DESTINATION share/dbus-1/system-services)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.conf DESTINATION /etc/dbus-1/system.d)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Services.policy DESTINATION share/polkit-1/actions)
endif(WITH-DBUS)
