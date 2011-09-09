SET(BROKER_DAEMON "matahari-brokerd")

if(WITH-QMF)
if(NOT WIN32)
    add_executable(${BROKER_DAEMON} broker.c broker_federation.c broker_${VARIANT}.c)

    if(HAVE_RESOLV_H)
        target_link_libraries(${BROKER_DAEMON} mcommon)
        target_link_libraries(${BROKER_DAEMON} resolv)
    endif(HAVE_RESOLV_H)

    install(FILES ${BROKER_DAEMON}.${AGENT_MAN_SECTION}.gz DESTINATION share/man/man${AGENT_MAN_SECTION})

    install(TARGETS ${BROKER_DAEMON} DESTINATION sbin)

endif(NOT WIN32)
endif(WITH-QMF)

