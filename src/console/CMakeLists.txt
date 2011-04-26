set(HOST_LIB "mconsole")
set(HOST_QMF "matahari-qmf-consoled")
set(HOST_DBUS "matahari-dbus-consoled")

# QMF daemon
if(WITH-QMF)
    add_executable(${CONSOLE_QMF} matahari-console.cpp)
    target_link_libraries(${CONSOLE_QMF} mcommon mcommon_qmf)

    target_link_libraries(${CONSOLE_QMF} ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(${CONSOLE_QMF} ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

    if(WIN32)
        target_link_libraries(${CONSOLE_QMF} wsock32)
    endif(WIN32)

    install(TARGETS ${CONSOLE_QMF} DESTINATION sbin)
endif(WITH-QMF)
