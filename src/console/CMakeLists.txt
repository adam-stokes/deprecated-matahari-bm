set(CONSOLE_CONFIG_QMF "matahari-console-configd")

# QMF daemon
if(WITH-QMF)
    add_executable(${CONSOLE_CONFIG_QMF} config.cpp)
    target_link_libraries(${CONSOLE_CONFIG_QMF} mcommon mcommon_qmf)

    target_link_libraries(${CONSOLE_CONFIG_QMF} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(${CONSOLE_CONFIG_QMF} ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${CONSOLE_CONFIG_QMF} wsock32)
    endif(WIN32)

    install(TARGETS ${CONSOLE_CONFIG_QMF} DESTINATION sbin)
endif(WITH-QMF)
