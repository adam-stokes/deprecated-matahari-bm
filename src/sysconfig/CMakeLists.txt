set(BASE "sysconfig")
set(BASE_LIB "m${BASE}")
set(QMF_AGENT "matahari-qmf-${BASE}d")
set(QMF_CONSOLE "matahari-qmf-${BASE}-consoled")

# QMF daemon
if(WITH-QMF)
    set(SCHEMA_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/qmf/org/matahariproject/QmfPackage.cpp)
    generate_qmf_schemas(${CMAKE_CURRENT_SOURCE_DIR}/schema.xml ${SCHEMA_SOURCES})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    
    add_executable(${QMF_AGENT} ${BASE}-qmf.cpp ${SCHEMA_SOURCES})
    add_executable(${QMF_CONSOLE} ${BASE}-console.cpp)

    target_link_libraries(${QMF_AGENT} ${BASE_LIB} mcommon_qmf)
    target_link_libraries(${QMF_CONSOLE} mcommon_qmf)

    if(NOT WIN32)
        configure_file(${CMAKE_SOURCE_DIR}/matahari.init.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE} DESTINATION ${initdir})
    	set(BASE_SUB "-console")
    	configure_file(${CMAKE_SOURCE_DIR}/matahari.init.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}${BASE_SUB})
    	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}${BASE_SUB} DESTINATION ${initdir})
    endif(NOT WIN32)
    
    install(TARGETS ${QMF_AGENT} DESTINATION sbin)
    install(TARGETS ${QMF_CONSOLE} DESTINATION sbin)
endif(WITH-QMF)
