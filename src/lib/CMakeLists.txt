# Library

set(MHOST_VERSION 0.0.1)
set(MSRV_VERSION 0.0.1)
set(MNET_VERSION 0.0.1)
set(MCOMMON_VERSION 0.0.1)
set(MCOMMON_QMF_VERSION 0.0.1)
set(MCOMMON_DBUS_VERSION 0.0.1)

set(HOST_LIB "mhost")
set(SRV_LIB "msrv")
set(NET_LIB "mnet")

add_library (${HOST_LIB} SHARED host.c host_${VARIANT}.c)
add_library (${SRV_LIB} SHARED services.c services_${VARIANT}.c)
add_library (${NET_LIB} SHARED network.c network_${VARIANT}.c)
add_library (mcommon SHARED utilities.c mainloop.c)

include_directories(${pcre_INCLUDE_DIRS})
target_link_libraries(${HOST_LIB} ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})
target_link_libraries(${SRV_LIB} ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})
target_link_libraries(${NET_LIB} ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})
target_link_libraries(mcommon ${SIGAR} ${glib_LIBRARIES})

set_target_properties(${HOST_LIB} PROPERTIES SOVERSION ${MHOST_VERSION})
set_target_properties(${SRV_LIB} PROPERTIES SOVERSION ${MSRV_VERSION})
set_target_properties(${NET_LIB} PROPERTIES SOVERSION ${MNET_VERSION})
set_target_properties(mcommon PROPERTIES SOVERSION ${MCOMMON_VERSION})

if(WIN32)
    target_link_libraries(${HOST_LIB} wsock32)
    target_link_libraries(${SRV_LIB} wsock32)
    target_link_libraries(${NET_LIB} wsock32)
    target_link_libraries(mcommon wsock32)

    install(TARGETS ${HOST_LIB} DESTINATION sbin)
    install(TARGETS ${SRV_LIB} DESTINATION sbin)
    install(TARGETS ${NET_LIB} DESTINATION sbin)
    install(TARGETS mcommon DESTINATION sbin)
else(WIN32)
    install(TARGETS ${HOST_LIB} DESTINATION lib${LIB_SUFFIX})
    install(TARGETS ${SRV_LIB} DESTINATION lib${LIB_SUFFIX})
    install(TARGETS ${NET_LIB} DESTINATION lib${LIB_SUFFIX})
    install(TARGETS mcommon DESTINATION lib${LIB_SUFFIX})
endif(WIN32)

if(WITH-QMF)
    add_library (mcommon_qmf SHARED mh_agent.cpp)

    target_link_libraries(mcommon_qmf ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    set_target_properties(mcommon_qmf PROPERTIES SOVERSION ${MCOMMON_QMF_VERSION})

    if(WIN32)
        target_link_libraries(mcommon_qmf wsock32)
        install(TARGETS mcommon_qmf DESTINATION sbin)
    else(WIN32)
        install(TARGETS mcommon_qmf DESTINATION lib${LIB_SUFFIX})
    endif(WIN32)
endif(WITH-QMF)


if(WITH-DBUS)
    add_library (mcommon_dbus SHARED mh_dbus_common.c)

    include_directories(${dbus-glib_INCLUDE_DIRS})
    include_directories(${polkit_INCLUDE_DIRS})

    target_link_libraries(mcommon_dbus ${polkit_LIBRARIES})
    set_target_properties(mcommon_dbus PROPERTIES SOVERSION ${MCOMMON_DBUS_VERSION})

    install(TARGETS mcommon_dbus DESTINATION lib${LIB_SUFFIX})
endif(WITH-DBUS)
