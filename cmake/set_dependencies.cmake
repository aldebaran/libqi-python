# This file finds the dependencies required by the project and creates the
# needed targets.
#
# It can only be included after a call to `project` and only once.
#
# After using CMake standard `find_package` modules, we often set the following
# variables (with <dep> the name of a dependency):
#   - <dep>_PACKAGE_FOUND
#   - <dep>_LIBRARIES
#   - <dep>_INCLUDE_DIRS
#
# These variables are the ones the qibuild CMake part uses in its internal
# functions and `find_package` modules (such as `qi_use_lib`). This allows us
# to use CMake standard modules and bypass qibuild own search procedures.
#
# Once this file is loaded, the following targets are expected to be defined:
#
#   - Threads::Threads, the thread library.
#   - OpenSSL::SSL, aka libssl.
#   - OpenSSL::Crypto, aka libcrypto.
#   - Python::Interpreter, the Python interpreter, if found.
#   - Python::Python, the Python library for embedding the interpreter.
#   - Python::Module, the Python library for modules.
#   - python_headers, header-only library for Python.
#   - Boost::headers, the target for all the header-only libraries.
#   - Boost::atomic, aka libboost_atomic.
#   - Boost::date_time, aka libboost_date_time.
#   - Boost::thread, aka libboost_thread.
#   - Boost::chrono, aka libboost_chrono.
#   - Boost::filesystem, aka libboost_filesystem.
#   - Boost::locale, aka libboost_locale.
#   - Boost::regex, aka libboost_regex.
#   - Boost::program_options, aka libboost_program_options.
#   - Boost::random, aka libboost_random.
#   - pybind11, a header-only Python binding library.
#   - gtest & gtest_main, the googletest library.
#   - gmock & gmock_main, the googlemock library.
#   - qi.interface, the high level target for linking with libqi.
#
# When needed, the following targets can also be defined:
#   - ICU::i18n, aka libicui18n.
#   - ICU::data, aka libicudata.
#   - ICU::uc, aka libicuuc.

include_guard(GLOBAL)

# Version of Boost to use. It will be used as the argument to the `find_package(Boost)` call.
overridable_variable(BOOST_VERSION 1.77)

# Version of pybind11 to use.
overridable_variable(PYBIND11_VERSION 2.5.0)

# URL of the git repository from which to download pybind11. For more details, see CMake
# `ExternalProject` module documentation of the `GIT_REPOSITORY` argument.
overridable_variable(PYBIND11_GIT_REPOSITORY https://github.com/pybind/pybind11)

# Git branch name, tag or commit hash to checkout for pybind11. For more details, see CMake
# `ExternalProject` module documentation of the `GIT_TAG` argument.
overridable_variable(PYBIND11_GIT_TAG v${PYBIND11_VERSION})

# Version of googletest to use.
overridable_variable(GOOGLETEST_VERSION 1.10.0)

# URL of the git repository from which to download googletest. For more details, see CMake
# `ExternalProject` module documentation of the `GIT_REPOSITORY` argument.
overridable_variable(GOOGLETEST_GIT_REPOSITORY https://github.com/google/googletest.git)

# Git branch name, tag or commit hash to checkout for googletest. For more details, see CMake
# `ExternalProject` module documentation of the `GIT_TAG` argument.
overridable_variable(GOOGLETEST_GIT_TAG release-${GOOGLETEST_VERSION})

set(PYTHON_VERSION_STRING "" CACHE STRING "Version of Python to look for. This \
variable can be specified by tools run directly from Python to enforce the \
use of the same version.")
mark_as_advanced(PYTHON_VERSION_STRING)

if(NOT COMMAND FetchContent_Declare)
  include(FetchContent)
endif()

# This part is a bit tricky. Some of our internal toolchains tend to confuse the
# CMake standard `find_package` modules, so we try to use some heuristics to
# set hints for these modules.
#
# If we're using a toolchain, it's most likely either one of our internal
# desktop or Yocto toolchain. This should cover most of our building cases, but
# not all of them, especially for users trying to build the library outside of
# the organization. For these, we let them manually specify the needed
# dependencies path.
if(CMAKE_TOOLCHAIN_FILE)
  message(VERBOSE "Toolchain file is set.")
  get_filename_component(QIPYTHON_TOOLCHAIN_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
  get_filename_component(QIPYTHON_TOOLCHAIN_DIR "${QIPYTHON_TOOLCHAIN_DIR}" ABSOLUTE)
  message(VERBOSE "Toolchain directory is ${QIPYTHON_TOOLCHAIN_DIR}.")

  # Cross-compiling for Yocto
  if(YOCTO_SDK_TARGET_SYSROOT)
    message(VERBOSE "Yocto cross compilation detected.")
    message(VERBOSE "Yocto target sysroot is '${YOCTO_SDK_TARGET_SYSROOT}'.")
    set(_yocto_target_sysroot_usr "${YOCTO_SDK_TARGET_SYSROOT}/usr")
    set(_openssl_root ${_yocto_target_sysroot_usr})
    set(_icu_root ${_yocto_target_sysroot_usr})
    set(_boost_root ${_yocto_target_sysroot_usr})
    set(_python_root ${_yocto_target_sysroot_usr})

  # Probably compiling for desktop
  else()
    message(VERBOSE "Assuming desktop compilation.")
    set(_openssl_root "${QIPYTHON_TOOLCHAIN_DIR}/openssl")
    set(_icu_root "${QIPYTHON_TOOLCHAIN_DIR}/icu")
    set(_boost_root "${QIPYTHON_TOOLCHAIN_DIR}/boost")
    # No definition of _python_root for desktop, we try to use the one
    # installed on the system.
  endif()

  # These variables are recognized by the associated CMake standard
  # `find_package` modules (see their specific documentation for details).
  #
  # Setting them as cache variable enables user customisation.
  set(OPENSSL_ROOT_DIR ${_openssl_root} CACHE PATH "root directory of an OpenSSL installation")
  set(ICU_ROOT ${_icu_root} CACHE PATH "the root of the ICU installation")
  set(BOOST_ROOT ${_boost_root} CACHE PATH "Boost preferred installation prefix")

  if(_python_root AND EXISTS "${_python_root}")
    # The `qibuild` `python3-config.cmake` module must be used to find the
    # dependency as it is packaged in our internal toolchains.
    # The CMake standard `find_package` module is named `Python3`, looking for
    # `PYTHON3` forces CMake to use the module that comes with `qibuild`.
    find_package(PYTHON3
      REQUIRED CONFIG
      NAMES PYTHON3 Python3 python3
      HINTS ${_python_root})

    if(PYTHON3_FOUND OR PYTHON3_PACKAGE_FOUND)
      # We want to set the `Python_LIBRARY` variable that the FindPython CMake module
      # recognizes, in order to instruct it where to find the library (the FindPython
      # module search mechanism uses the python-config utility which is not packaged
      # well enough in our toolchains for it to work, therefore we have to manually
      # tell the module where the library is).
      #
      # The `qibuild` Python3 module fills the PYTHON3_LIBRARIES variable with the
      # format "general;<path>;debug;<path>", which is recognized by
      # `target_link_libraries` (see the function documentation for
      # details). However, the FindPython CMake module expects an absolute path.
      #
      # Therefore we create an interface library `Python_interface`, we use
      # `target_link_libraries` so that the specific format is parsed, and then
      # we get the full path of the library from the `LINK_LIBRARIES` property of
      # the target.
      add_library(Python_interface INTERFACE)
      target_link_libraries(Python_interface INTERFACE ${PYTHON3_LIBRARIES})
      get_target_property(Python_LIBRARY Python_interface INTERFACE_LINK_LIBRARIES)
      list(GET PYTHON3_INCLUDE_DIRS 0 Python_INCLUDE_DIR)
    endif()
  endif()
endif()


# C++11
add_library(cxx11 INTERFACE)
target_compile_features(cxx11 INTERFACE cxx_std_11)


# Threads
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
  set(PTHREAD_PACKAGE_FOUND TRUE)
  set(PTHREAD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
  set(PTHREAD_INCLUDE_DIRS)
endif()


# OpenSSL
find_package(OpenSSL REQUIRED)


# Python
parse_python_version(_python_version "${PYTHON_VERSION_STRING}")
if(_python_version_VERSION_STRING)
  set(_python_version ${_python_version_VERSION_RELEASE_MAJOR_MINOR})
else()
  set(_python_version 3.5) # default to Python 3.5+.
endif()

# Set variables that the CMake module recognizes from variables that the
# scikit-build build system module may pass to us.
if(PYTHON_LIBRARY)
  set(Python_LIBRARY "${PYTHON_LIBRARY}")
endif()
if(PYTHON_INCLUDE_DIR)
  set(Python_INCLUDE_DIR "${PYTHON_INCLUDE_DIR}")
endif()

# First search for Python with Interpreter+Devel components, to allow the module
# to look for the interpreter that goes with the Python development library.
# Then if it could not find both, try looking for the development component
# only, but this time force the module to find it or fail (REQUIRED).
# Some of our toolchains (when crosscompiling for instance) have the Python
# library but not an interpreter that can run on the host.
find_package(Python ${_python_version} COMPONENTS Development Interpreter)
if(NOT Python_FOUND)
  find_package(Python ${_python_version} REQUIRED COMPONENTS Development)
endif()

list(GET Python_LIBRARIES 0 _py_lib)
add_library(python_headers INTERFACE)
target_include_directories(python_headers INTERFACE ${Python_INCLUDE_DIRS})
if(_py_lib MATCHES "python([23])\\.([0-9]+)([mu]*d[mu]*)")
  target_compile_definitions(python_headers INTERFACE Py_DEBUG)
endif()


# Boost
set(_boost_comps atomic date_time thread chrono filesystem locale regex
                 program_options random)
find_package(Boost ${BOOST_VERSION} EXACT REQUIRED COMPONENTS ${_boost_comps})

set(BOOST_PACKAGE_FOUND TRUE)
set(BOOST_LIBRARIES ${Boost_LIBRARIES})
set(BOOST_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
# Also set the "qibuild variables" for each of the components of boost.
foreach(_comp IN LISTS _boost_comps)
  string(TOUPPER ${_comp} _uc_comp)
  set(BOOST_${_uc_comp}_PACKAGE_FOUND TRUE)
  set(BOOST_${_uc_comp}_LIBRARIES ${Boost_${_uc_comp}_LIBRARY})
  set(BOOST_${_uc_comp}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
endforeach()


# ICU. These are the components that might be needed by Boost::locale.
find_package(ICU COMPONENTS i18n uc data)
set(ICU_PACKAGE_FOUND ${ICU_FOUND})
if(ICU_FOUND)
  target_link_libraries(Boost::locale INTERFACE ICU::i18n ICU::data ICU::uc)
endif()


# pybind11, which is a header-only library.
# It's simpler to just use it as a CMake external project (through FetchContent)
# and download it directly into the build directory, than to try to use a
# preinstalled version through `find_package`
FetchContent_Declare(pybind11
  GIT_REPOSITORY ${PYBIND11_GIT_REPOSITORY}
  GIT_TAG        ${PYBIND11_GIT_TAG})
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  # pybind11 build system and targets do a lot of stuff automatically (such as
  # looking for Python libs, adding the C++11 flag, ...). We prefer to do things
  # ourselves, so we recreate a target directly instead of adding it as a
  # subproject.
  add_library(pybind11 INTERFACE)
  target_include_directories(pybind11 INTERFACE ${pybind11_SOURCE_DIR}/include)
  target_link_libraries(pybind11 INTERFACE cxx11 python_headers)
endif()


# googletest
#
# This is the simplest way to depend on googletest & googlemock. As they are
# fairly quick to compile, we can have it as a subproject (as a `FetchContent`
# project).
FetchContent_Declare(googletest
  GIT_REPOSITORY ${GOOGLETEST_GIT_REPOSITORY}
  GIT_TAG        ${GOOGLETEST_GIT_TAG})
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  set(INSTALL_GTEST OFF)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

set(GTEST_PACKAGE_FOUND TRUE)
set(GMOCK_PACKAGE_FOUND TRUE)
set(GTEST_LIBRARIES $<TARGET_FILE:gtest>)
set(GMOCK_LIBRARIES $<TARGET_FILE:gmock>)

# Unfortunately, we cannot get the `INTERFACE_INCLUDE_DIRECTORIES` of the gtest
# or gmock targets as they might contain generator expressions containing ";"
# characters, which qibuild wrongly splits as elements of a list. Instead, we
# have to build the paths manually.
set(GTEST_INCLUDE_DIRS ${googletest_SOURCE_DIR}/googletest/include)
set(GMOCK_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS}
                       ${googletest_SOURCE_DIR}/googlemock/include)


# LibQi
include(set_libqi_dependency)

# Generate a list of dependency directories that can be used for
# instance to generate RPATH or install those dependencies.
set(QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS ${Boost_LIBRARY_DIRS})
foreach(_lib IN LISTS OPENSSL_LIBRARIES ICU_LIBRARIES)
  if(EXISTS ${_lib})
    get_filename_component(_dir ${_lib} DIRECTORY)
    list(APPEND QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS ${_dir})
  endif()
endforeach()
list(REMOVE_DUPLICATES QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS)

# Adds all the dependency directories as RPATH for a target ouput
# binary in the build directory, so that we may for instance execute
# it directly.
function(set_build_rpath_to_qipython_dependencies target)
  message(VERBOSE
    "Setting ${target} build RPATH to '${QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS}'")
  set_property(TARGET ${target} PROPERTY
    BUILD_RPATH ${QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS})
endfunction()
