
find_path(MATAHARI_INCLUDE_DIR mh_agent.h
    HINTS
    $ENV{MATAHARI_DIR}
    PATH_SUFFIXES include/matahari include
    PATHS
    /usr
    /usr/local
)

find_library(MATAHARI_LIBRARY
    NAMES mcommon
    HINTS
    $ENV{MATAHARI_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
    /usr
    /usr/local
)

find_library(MATAHARI_QMF_LIBRARY
    NAMES mcommon_qmf
    HINTS
    $ENV{MATAHARI_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
    /usr
    /usr/local
)

find_library(MATAHARI_DBUS_LIBRARY
    NAMES mcommon_dbus
    HINTS
    $ENV{MATAHARI_DIR}
    PATH_SUFFIXES lib64 lib
    PATHS
    /usr
    /usr/local
)

set(MATAHARI_QMF_LIBRARIES ${MATAHARI_LIBRARY} ${MATAHARI_QMF_LIBRARY})
set(MATAHARI_DBUS_LIBRARIES ${MATAHARI_LIBRARY} ${MATAHARI_DBUS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Matahari DEFAULT_MSG MATAHARI_QMF_LIBRARIES MATAHARI_DBUS_LIBRARIES MATAHARI_INCLUDE_DIR)

mark_as_advanced(MATAHARI_INCLUDE_DIR MATAHARI_QMF_LIBRARIES MATAHARI_DBUS_LIBRARIES)
