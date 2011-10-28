
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.conf.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.conf @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-broker.sysconf.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.sysconf.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari.sysconf @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-vios-proxy-host.init.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-host.init @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-vios-proxy-guest.init.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-guest.init @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-vios-proxy.sysconf.in
               ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy.sysconf @ONLY)

install(DIRECTORY DESTINATION ${localstatedir}/lib/matahari
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                              GROUP_READ GROUP_WRITE GROUP_EXECUTE)


install(CODE "execute_process(COMMAND chown root:qpidd ${localstatedir}/lib/matahari)")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari.sysconf
        DESTINATION ${sysconfdir}/sysconfig RENAME matahari)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf
        DESTINATION ${sysconfdir}/sysconfig RENAME matahari-broker)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.conf
        DESTINATION ${sysconfdir})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy.sysconf
        DESTINATION ${sysconfdir}/sysconfig RENAME matahari-vios-proxy)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-host.init
        DESTINATION ${initdir} RENAME matahari-vios-proxy-host)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-guest.init
        DESTINATION ${initdir} RENAME matahari-vios-proxy-guest)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/matahari-guest-agent.rules
        DESTINATION ${sysconfdir}/udev/rules.d
        RENAME 99-matahari-guest-agent.rules)

if(WITH-DEBIAN)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari.sysconf
            DESTINATION ${sysconfdir}/default RENAME matahari)
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.sysconf
            DESTINATION ${sysconfdir}/default RENAME matahari-broker)
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-broker.conf
            DESTINATION ${sysconfdir})
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy.sysconf
            DESTINATION ${sysconfdir}/default RENAME matahari-vios-proxy)
    
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/matahari-guest-agent.rules
            DESTINATION ${sysconfdir}/udev/rules.d
            RENAME 99-matahari-guest-agent.rules)
endif(WITH-DEBIAN)

# Check for systemd
include(FindPkgConfig)
pkg_check_modules(systemd systemd)

if(systemd_FOUND)
    set(systemdunitdir "/lib/systemd/system")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-vios-proxy-host.service.in
                   ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-host.service)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari-vios-proxy-guest.service.in
                   ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-guest.service)


    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-host.service
            DESTINATION ${systemdunitdir})

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-vios-proxy-guest.service
            DESTINATION ${systemdunitdir})

endif(systemd_FOUND)

