set(BASE "config")
set(BASE_LIB "m${BASE}")
set(QMF_AGENT "matahari-qmf-${BASE}d")

# QMF daemon
if(WITH-QMF)
    add_executable(${QMF_AGENT} ${BASE}-qmf.cpp ${SCHEMA_SOURCES})
    target_link_libraries(${QMF_AGENT} mcommon mcommon_qmf mnetwork mhost)

    target_link_libraries(${QMF_AGENT} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(${QMF_AGENT} ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${QMF_AGENT} wsock32)
    endif(WIN32)

    install(TARGETS ${QMF_AGENT} DESTINATION sbin)
endif(WITH-QMF)
