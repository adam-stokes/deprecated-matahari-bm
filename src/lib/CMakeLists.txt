# Library

set(MHOST_VERSION 0.0.1)
set(MSRV_VERSION 0.0.1)
set(MNET_VERSION 0.0.1)
set(MCOMMON_VERSION 0.0.1)

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

set_target_properties(${HOST_LIB} PROPERTIES SOVERSION ${MHOST_VERSION})
set_target_properties(${SRV_LIB} PROPERTIES SOVERSION ${MSRV_VERSION})
set_target_properties(${NET_LIB} PROPERTIES SOVERSION ${MNET_VERSION})
set_target_properties(mcommon PROPERTIES SOVERSION ${MCOMMON_VERSION})

if(WIN32)
    target_link_libraries(${HOST_LIB} wsock32)
    target_link_libraries(${SRV_LIB} wsock32)
    target_link_libraries(${NET_LIB} wsock32)

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
