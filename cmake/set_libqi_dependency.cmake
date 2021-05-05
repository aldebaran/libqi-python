set(LIBQI_VERSION 1.8.5
    CACHE STRING
    "Version of LibQi to use. If not empty, it will be checked against the \
version in the `package.xml` file in the libqi sources.")

# Our github clone is sometimes late or is missing tags, so we enable
# customisation of the URL at configuration time, so users can use another clone.
set(LIBQI_GIT_REPOSITORY https://github.com/aldebaran/libqi.git
    CACHE STRING
    "URL of the git repository from which to download LibQi. For more details, \
see CMake `ExternalProject` module documentation of the `GIT_REPOSITORY` \
argument.")

set(LIBQI_GIT_TAG qi-framework-v${LIBQI_VERSION}
    CACHE STRING
    "Git branch name, tag or commit hash to checkout for LibQi. \
For more details, see CMake `ExternalProject` module documentation of the \
`GIT_TAG` argument.")


if(LIBQI_VERSION)
  message(STATUS "LibQi: expected version is \"${LIBQI_VERSION}\"")
endif()

# Checks that the version present in the `package_xml_file` file is the same as
# the one specified in the `LIBQI_VERSION` cache variable, if it is set.
function(check_libqi_version package_xml_file version_var)
  file(STRINGS ${package_xml_file} _package_xml_strings REGEX version)
  set(_package_version "")
  if(_package_xml_strings MATCHES [[version="([^"]*)"]])
    set(_package_version ${CMAKE_MATCH_1})
    message(STATUS "LibQi: found version \"${_package_version}\" from file '${package_xml_file}'")
    if(LIBQI_VERSION AND NOT (LIBQI_VERSION VERSION_EQUAL _package_version))
      message(FATAL_ERROR "LibQi version mismatch (expected \
  \"${LIBQI_VERSION}\", found \"${_package_version}\")")
    endif()
  else()
    set(_msg_type WARNING)
    if (LIBQI_VERSION)
      set(_msg_type FATAL_ERROR)
    endif()
    message(${_msg_type} "Could not read LibQi version from file \
${package_xml_file}. Please check that the file contains a version \
attribute.")
  endif()
  set(${version_var} ${_package_version} PARENT_SCOPE)
endfunction()

if(QIPYTHON_STANDALONE)
  include(set_libqi_dependency_standalone)
else()
  include(set_libqi_dependency_system)
endif()

target_link_libraries(qi.interface INTERFACE cxx11)

# Generate a Python file containing information about the native part of the
# module.\
set(NATIVE_VERSION "${LIBQI_PACKAGE_VERSION}")
if(NOT NATIVE_VERSION)
  set(NATIVE_VERSION "unknown")
endif()
set(QIPYTHON_NATIVE_PYTHON_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/${QIPYTHON_PYTHON_MODULE_NAME}/native.py")
configure_file(cmake/native.py.in "${QIPYTHON_NATIVE_PYTHON_FILE}" @ONLY)
