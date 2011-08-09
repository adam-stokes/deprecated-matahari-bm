set(BASE "host")
set(BASE_LIB "m${BASE}")
set(QMF_AGENT "matahari-qmf-${BASE}d")
set(DBUS_AGENT "matahari-dbus-${BASE}d")

# QMF daemon
if(WITH-QMF)
    set(SCHEMA_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/QmfPackage.cpp)
    generate_qmf_schemas(${CMAKE_CURRENT_SOURCE_DIR}/schema.xml ${SCHEMA_SOURCES})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    add_executable(${QMF_AGENT} ${BASE}-qmf.cpp ${SCHEMA_SOURCES})
    target_link_libraries(${QMF_AGENT} ${BASE_LIB} mcommon_qmf)

    if(NOT WIN32)
        configure_file(${CMAKE_SOURCE_DIR}/matahari.init.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE} DESTINATION ${initdir})
    endif(NOT WIN32)

    install(TARGETS ${QMF_AGENT} DESTINATION sbin)

    if(systemd_FOUND)
        configure_file(${CMAKE_SOURCE_DIR}/matahari.service.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}.service)
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}.service DESTINATION ${systemdunitdir})
    endif(systemd_FOUND)
    
endif(WITH-QMF)


# DBus daemon
if(WITH-DBUS)
    # Auto-generated stuff
    generate_dbus_headers(${BASE} ${CMAKE_CURRENT_BINARY_DIR}/org.matahariproject.Host.xml)
    # Create src/dbus directory and generate DBus XML definition files into it
    generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/schema.xml)

    # Check if all properties|statistics|methods have PolicyKit action defined
    check_policies_for_schema(${CMAKE_CURRENT_SOURCE_DIR}/schema.xml)

    # Must be included to find matahari-API-dbus-properties.h
    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    set(DBUS_AGENT_SOURCE 
	${BASE}-dbus.c
	${BASE}-dbus-glue.h 
	${BASE}-dbus-properties.h
    )

    add_executable(${DBUS_AGENT} ${DBUS_AGENT_SOURCE})

    target_link_libraries(${DBUS_AGENT} ${BASE_LIB} mcommon_dbus ${dbus-glib_LIBRARIES})
    include_directories(${dbus-glib_INCLUDE_DIRS})

    # Install targets
    # TODO: fix hardcoded paths, should go to libexec
    install(TARGETS ${DBUS_AGENT} DESTINATION sbin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.matahariproject.Host.xml DESTINATION share/dbus-1/interfaces)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.service DESTINATION share/dbus-1/system-services)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.conf DESTINATION /etc/dbus-1/system.d)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.matahariproject.Host.policy DESTINATION share/polkit-1/actions)
endif(WITH-DBUS)
