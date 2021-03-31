cmake_minimum_required(VERSION 3.16)

if(ANDROID)
  message(FATAL_ERROR "This project does not support Android, stopping.")
endif()

# Find our project CMake files in the dedicated subdirectory.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(utils)
include(set_globals)

# Read the version of the library directly from the python file and sets it
# as the version of the project.
#
# The file is expected to contain a line `__version__ = <version>`.
set(_version_file ${CMAKE_CURRENT_SOURCE_DIR}/qi/_version.py)
file(STRINGS ${_version_file} _version_strings REGEX version)
set(_version_regex "__version__[ \t]*=[ \t]*['\"]([^'\"]+)['\"]")
if(NOT _version_strings MATCHES "${_version_regex}")
  message(FATAL_ERROR "Could not find the package version in file '${_version_file}'.")
endif()
parse_python_version(_version ${CMAKE_MATCH_1})

project(libqi-python VERSION ${_version_VERSION_RELEASE})

option(QIPYTHON_STANDALONE
       "If set, builds and installs the module as a standalone, ready to be \
packaged as a wheel. Otherwise, builds and installs it, ready to be included \
as part of a system (such as NAOqi) that contains its dependencies."
       FALSE)

# Including this package creates the common options/flags/functions of qi projects.
find_package(qibuild)

include(set_dependencies)
include(add_targets)
include(set_install_rules)
include(add_tests)
