SET(BROKER_DAEMON "matahari-brokerd")

if(WITH-QMF)
    add_executable(${BROKER_DAEMON} broker.c broker_federation.c broker_${VARIANT}.c)
    target_link_libraries(${BROKER_DAEMON} ${glib_LIBRARIES} mcommon)

    if(HAVE_RESOLV_H)
        target_link_libraries(${BROKER_DAEMON} resolv)
    endif(HAVE_RESOLV_H)

    install(TARGETS ${BROKER_DAEMON} DESTINATION sbin)


    SET(MAN_PAGE ${BROKER_DAEMON}.${AGENT_MAN_SECTION})
    install(CODE "execute_process(COMMAND gzip
        INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${MAN_PAGE}
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MAN_PAGE}.gz)")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MAN_PAGE}.gz
        DESTINATION share/man/man${AGENT_MAN_SECTION})

    configure_file(${CMAKE_SOURCE_DIR}/sys/matahari-broker.init.in
                   ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.init @ONLY)

    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.init
            DESTINATION ${initdir} RENAME matahari-broker)

    pkg_check_modules(systemd systemd)

    if(systemd_FOUND)
        configure_file(${CMAKE_SOURCE_DIR}/sys/matahari-broker.service.in
                       ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.service)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.service DESTINATION ${systemdunitdir})
    endif(systemd_FOUND)

endif(WITH-QMF)

