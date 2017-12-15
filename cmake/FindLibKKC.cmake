#.rst:
# FindLibKKC
# -----------
#
# Try to find the LibKKC library
#
# Once done this will define
#
# ::
#
#   LIBKKC_FOUND - System has LibKKC
#   LIBKKC_INCLUDE_DIR - The LibKKC include directory
#   LIBKKC_LIBRARIES - The libraries needed to use LibKKC
#   LIBKKC_DEFINITIONS - Compiler switches required for using LibKKC
#   LIBKKC_VERSION_STRING - the version of LibKKC found (since CMake 2.8.8)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LibKKC QUIET libkkc)

find_path(LIBKKC_INCLUDE_DIR NAMES libkkc/libkkc.h
   HINTS
   ${PC_LIBKKC_INCLUDEDIR}
   ${PC_LIBKKC_INCLUDE_DIRS}
   )

find_library(LIBKKC_LIBRARIES NAMES kkc
   HINTS
   ${PC_LIBKKC_LIBDIR}
   ${PC_LIBKKC_LIBRARY_DIRS}
   )

if(PC_LIBKKC_VERSION)
    set(LIBKKC_VERSION_STRING ${PC_LIBKKC_VERSION})
endif()

# handle the QUIETLY and REQUIRED arguments and set LIBKKC_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibKKC
                                  REQUIRED_VARS LIBKKC_LIBRARIES LIBKKC_INCLUDE_DIR
                                  VERSION_VAR LIBKKC_VERSION_STRING)

mark_as_advanced(LIBKKC_INCLUDE_DIR LIBKKC_LIBRARIES)

if (LIBKKC_FOUND AND NOT TARGET LibKKC::LibKKC)
    add_library(LibKKC::LibKKC UNKNOWN IMPORTED)
    set_target_properties(LibKKC::LibKKC PROPERTIES
        IMPORTED_LOCATION "${LIBKKC_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBKKC_INCLUDE_DIR}"
    )
endif()

