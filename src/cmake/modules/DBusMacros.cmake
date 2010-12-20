# Generator for files specific to DBus interface

# This macro takes path to schema.xml that describes provided interface
# Second argument is list of API names for which will be generated c interface
# files
macro(generate_dbus_interfaces SCHEMA APIS)
    find_file(XSLTPROC xsltproc)
    find_file(BIND_TOOL dbus-binding-tool)

    # Create list of generated dbus interface files
    set(XMLS "")
    foreach(API ${APIS})
        set(XMLS ${XMLS} ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.matahariproject.${API}.xml)
    endforeach(API ${APIS})
    message(${XMLS})

    # Convert schema.xml to dbus interfaces
    add_custom_command(
        OUTPUT ${XMLS}
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/dbus/
        COMMAND cd dbus && ${XSLTPROC} ${CMAKE_CURRENT_SOURCE_DIR}/schema-to-dbus.xsl ${SCHEMA}
        DEPENDS ${SCHEMA}
        COMMENT "Processing schema.xml with schema-to-dbus.xsl"
    )


    # Convert dbus interface to glue and properties files
    foreach(API ${APIS})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-glue.h
            COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${API}/
            COMMAND ${BIND_TOOL} --prefix=fmci --mode=glib-server
                --output=${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-glue.h
                ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.matahariproject.${API}.xml
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.matahariproject.${API}.xml
            COMMENT "Generating matahari-${API}-dbus-glue.h"
        )

        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-properties.h
            COMMAND ${XSLTPROC} --output ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-properties.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dbus-to-c.xsl
                ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.matahariproject.${API}.xml
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dbus/org.matahariproject.${API}.xml
            COMMENT "Generating matahari-${API}-dbus-properties.h"
        )
    endforeach(API ${APIS})

endmacro(generate_dbus_interfaces)
