# Generator for files specific to DBus interface

# This macro takes name of the API and then creates two files
# matahari-${API}-dbus-glue.h and matahari-${API}-dbus-properties.h
# First one contains gobject definition generated by dbus-binding-tool
# Second one is definition of properties of the object
macro(generate_dbus_headers API XML)
    find_file(XSLTPROC xsltproc)
    find_file(BIND_TOOL dbus-binding-tool)
    find_file(DBUS_TO_C dbus-to-c.xsl
              ${CMAKE_CURRENT_SOURCE_DIR}/..
              /usr/share/matahari)

    # Convert dbus interface to glue and properties files
    add_custom_command(
        OUTPUT ${API}-dbus-glue.h
        COMMAND ${BIND_TOOL} --prefix=matahari --mode=glib-server
            --output=${API}-dbus-glue.h
            ${XML}
        COMMENT "Generating ${API}-dbus-glue.h"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${API}-dbus-properties.h
        COMMAND ${XSLTPROC} --output ${API}-dbus-properties.h
            ${DBUS_TO_C}
            ${XML}
        COMMENT "Generating ${API}-dbus-properties.h"
        VERBATIM
    )
endmacro(generate_dbus_headers)

# This macro will transform QMF interface XML to DBus interface XML file
# Argument SCHEMA - path to XML schema file
macro(generate_dbus_interface SCHEMA)
    find_file(SCHEMA_TO_DBUS schema-to-dbus.xsl
              ${CMAKE_CURRENT_SOURCE_DIR}/.. /usr/share/matahari)
    find_file(XSLTPROC xsltproc)
    # Generate files
    execute_process(
        COMMAND ${XSLTPROC} ${SCHEMA_TO_DBUS} ${SCHEMA}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endmacro(generate_dbus_interface)

# This macro will generate QMF definition files from QMF schema
# Argument SCHEMAS - paths to XML schema files
# Variable SCHEMA_SOURCES - list of generated cpp files (it can be added
#                           to add_executable command for proper compliation)
macro(generate_qmf_schemas SCHEMAS SCHEMA_SOURCES)
    set(regen_schema OFF)

    if (SCHEMA_SOURCES)
        # There are some already generated files, check if they
        # need to be regenerated
        foreach (schema_file ${SCHEMAS})
            foreach (file ${SCHEMA_SOURCES})
                if (EXISTS ${file})
                    if (${schema_file} IS_NEWER_THAN ${file})
                        set(regen_schema ON)
                        message("${schema_file} is newer than ${file}")
                    endif (${schema_file} IS_NEWER_THAN ${file})
                else (EXISTS ${file})
                    set(regen_schema ON)
                endif (EXISTS ${file})
            endforeach (file ${SCHEMA_SOURCES})
        endforeach (schema_file ${SCHEMAS})
    else (SCHEMA_SOURCES)
        # No already generated files => generate them
        set(regen_schema ON)
    endif (SCHEMA_SOURCES)

    if (regen_schema)
        # Regenerate everything
	    message(STATUS "Regenerating QMFv2 classes from: ${SCHEMAS}")
        execute_process(COMMAND rm -f ${SCHEMA_SOURCES})
        message("${QMFGEN} -2 -o ./qmf ${SCHEMAS}  in ${CMAKE_CURRENT_BINARY_DIR}")
        execute_process(COMMAND ${QMFGEN} -2 -o ./qmf ${SCHEMAS}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    else (regen_schema)
        message(STATUS "No need to generate QMFv2 classes from: ${SCHEMAS}")
    endif (regen_schema)
endmacro(generate_qmf_schemas)


# This macro takes schema XML file and check if there are PolicyKit action
# for each property/statistic/method in .policy file in current directory.
# Name of the file must be (Package from schema).(Class from schema).policy
macro(check_policies_for_schema SCHEMA)
    find_file(XSLTPROC xsltproc)
    find_file(CHECK_POLICY check-policy.xsl
              ${CMAKE_CURRENT_SOURCE_DIR}/..
              /usr/share/matahari)
    execute_process(COMMAND ${XSLTPROC} --path . --novalid ${CHECK_POLICY} ${SCHEMA}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    ERROR_VARIABLE OUTPUT)
    if (NOT ${OUTPUT} EQUAL "")
        message(AUTHOR_WARNING "Policy file is missing some actions:\n" ${OUTPUT})
    endif (NOT ${OUTPUT} EQUAL "")
endmacro(check_policies_for_schema)

macro(create_manpage BINARY SECTION DESC)
    if(HELP2MAN)
        add_custom_command(
            TARGET ${BINARY}
            POST_BUILD
            COMMAND ${HELP2MAN} --no-info --section ${SECTION} --name ${DESC} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY} | gzip > ${BINARY}.${SECTION}.gz 
            COMMENT "Generating ${BINARY} man page"
            VERBATIM
        )
    else(HELP2MAN)
        if(REQUIRE_HELP2MAN)
           message(FATAL_ERROR "help2man is required.")
        endif(REQUIRE_HELP2MAN)
    endif(HELP2MAN)
    if(NOT WIN32)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BINARY}.${SECTION}.gz DESTINATION share/man/man${SECTION})
    endif(NOT WIN32)
endmacro(create_manpage)

macro(create_service_scripts BASE)
    if(NOT WIN32)
        configure_file(${CMAKE_SOURCE_DIR}/sys/matahari.init.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE})
        install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE} DESTINATION ${initdir})
    endif(NOT WIN32)

    if(systemd_FOUND)
        configure_file(${CMAKE_SOURCE_DIR}/sys/matahari.service.in ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}.service)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}.service DESTINATION ${systemdunitdir})
    endif(systemd_FOUND)
    
    if(IS_DIRECTORY /lib/init)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/matahari.upstart.in
                       ${CMAKE_CURRENT_BINARY_DIR}/matahari-${BASE}.upstart)
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/matahari-${BASE}.upstart
                DESTINATION ${sysconfdir}/init
                RENAME matahari-${BASE}.conf
    endif(IS_DIRECTORY /lib/init)
endmacro(create_service_scripts)
