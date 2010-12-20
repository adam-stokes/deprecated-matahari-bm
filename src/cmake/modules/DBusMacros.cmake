# Generator for files specific to DBus interface

macro(generate_dbus_interfaces SCHEMA APIS)
    find_file(XSLTPROC xsltproc)
    find_file(BIND_TOOL dbus-binding-tool)

    #set(XMLs 
    
    #add_custom_target(matahari-dbus
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dbus/matahari-host-dbus.xml
        COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/dbus/
        COMMAND cd dbus && ${XSLTPROC} ${CMAKE_CURRENT_SOURCE_DIR}/schema-to-dbus.xsl ${SCHEMA}
        DEPENDS ${SCHEMA}
        COMMENT "Processing schema.xml with schema-to-dbus.xsl"
    )

    foreach(API ${APIS})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-glue.h
            COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${API}/
            COMMAND ${BIND_TOOL} --prefix=fmci --mode=glib-server
                --output=${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-glue.h
                ${CMAKE_CURRENT_BINARY_DIR}/dbus/matahari-${API}-dbus.xml
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dbus/matahari-${API}-dbus.xml
            COMMENT "Generating matahari-${API}-dbus-glue.h"
        )

        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-properties.h
            COMMAND ${XSLTPROC} --output ${CMAKE_CURRENT_BINARY_DIR}/${API}/matahari-${API}-dbus-properties.h
                ${CMAKE_CURRENT_SOURCE_DIR}/dbus-to-c.xsl
                ${CMAKE_CURRENT_BINARY_DIR}/dbus/matahari-${API}-dbus.xml
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dbus/matahari-${API}-dbus.xml
            COMMENT "Generating matahari-${API}-dbus-properties.h"
        )

    endforeach(API ${APIS})

endmacro(generate_dbus_interfaces)
