# Library

set(LIBS mcommon mhost mnetwork mservice msysconfig)
include_directories(${pcre_INCLUDE_DIRS})

# See http://sourceware.org/autobook/autobook/autobook_91.html for
# how to set the library version appropriately

add_library (mcommon SHARED utilities.c utilities_${VARIANT}.c mainloop.c dnssrv.c dnssrv_${VARIANT}.c)
set_target_properties(mcommon PROPERTIES SOVERSION 1.0.0)
target_link_libraries(mcommon ${SIGAR} ${glib_LIBRARIES})

if(HAVE_RESOLV_H)
    target_link_libraries(mcommon resolv)
endif(HAVE_RESOLV_H)

add_library (mhost SHARED host.c host_${VARIANT}.c)
set_target_properties(mhost PROPERTIES SOVERSION 1.0.0)
target_link_libraries(mhost ${uuid_LIBRARIES} ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})

add_library (mnetwork SHARED network.c  network_${VARIANT}.c)
set_target_properties(mnetwork PROPERTIES SOVERSION 1.0.0)
target_link_libraries(mnetwork ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})

add_library (mservice SHARED services.c services_${VARIANT}.c)
set_target_properties(mservice PROPERTIES SOVERSION 1.0.0)
target_link_libraries(mservice ${pcre_LIBRARIES} mcommon ${SIGAR} ${glib_LIBRARIES})

add_library (msysconfig SHARED sysconfig.c sysconfig_${VARIANT}.c)
set_target_properties(msysconfig PROPERTIES SOVERSION 1.0.0)
target_link_libraries(msysconfig ${pcre_LIBRARIES} mcommon ${glib_LIBRARIES})

if(WIN32)
    foreach (lib ${LIBS})
        target_link_libraries(${lib} wsock32 dnsapi rpcrt4)
        install(TARGETS ${lib} DESTINATION sbin)
    endforeach (lib ${LIBS})
else(WIN32)
    foreach (lib ${LIBS})
	target_link_libraries(${lib} curl)
        install(TARGETS ${lib} DESTINATION lib${LIB_SUFFIX})
    endforeach (lib ${LIBS})
endif(WIN32)

if(WITH-QMF)
    add_library (mcommon_qmf SHARED mh_agent.cpp)
    set_target_properties(mcommon_qmf PROPERTIES SOVERSION 1.0.0)

    target_link_libraries(mcommon_qmf mcommon ${QPIDCOMMON_LIBRARY} ${QPIDCLIENT_LIBRARY} ${QPIDMESSAGING_LIBRARY} ${QMF2_LIBRARY})
    if(QPIDTYPES_LIBRARY)
        target_link_libraries(mcommon_qmf ${QPIDTYPES_LIBRARY})
    endif(QPIDTYPES_LIBRARY)

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
    set_target_properties(mcommon_dbus PROPERTIES SOVERSION 1.0.0)

    install(TARGETS mcommon_dbus DESTINATION lib${LIB_SUFFIX})
endif(WITH-DBUS)
