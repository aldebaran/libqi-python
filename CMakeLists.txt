## Copyright (c) 2023 Aldebaran Robotics. All rights reserved.
## Use of this source code is governed by a BSD-style license that can be
## found in the COPYING file.
##
## qi_python CMake project
## =======================
##
## Parameters:
## -----------
## BUILD_TESTING
## ~~~~~~~~~~~~~
## If set to true, enables building the tests. See the documentation of the
## `CTest` module for more details on this variable.

cmake_minimum_required(VERSION 3.23)

if(ANDROID)
  message(FATAL_ERROR "This project does not support Android, stopping.")
endif()

include(cmake/ParsePythonVersion.cmake)

# Read the version of the library directly from the `pyproject.toml` file and sets it
# as the version of the project.
set(pyproject_file "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml")
file(STRINGS ${pyproject_file} version_strings REGEX version)
set(version_regex "version[ \t]*=[ \t]*\"([^\"]+)\"")
if(NOT version_strings MATCHES "${version_regex}")
  message(FATAL_ERROR "Could not find the package version in file '${pyproject_file}'.")
endif()
parse_python_version(QI_PYTHON "${CMAKE_MATCH_1}")

project(qi_python VERSION "${QI_PYTHON_VERSION_RELEASE}")

include(cmake/BuildType.cmake)

# Enable testing with CTest. This defines the BUILD_TESTING option.
# Disable tests by default when cross compiling.
set(build_testing_default TRUE)
if(CMAKE_CROSSCOMPILING)
  set(build_testing_default FALSE)
endif()
option(BUILD_TESTING "Build the testing tree." "${build_testing_default}")
include(CTest)

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

# Output everything directly in predefined directories of the build tree.
# This is required by the SDK layout implementation.
# Also write a "path.conf" file, which is required for executing some tests.
set(sdk_dir "${CMAKE_BINARY_DIR}/sdk")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${sdk_dir}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${sdk_dir}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${sdk_dir}/lib")
set(path_conf_dir "${sdk_dir}/share/qi")
set(path_conf_file_path "${path_conf_dir}/path.conf")
if(NOT EXISTS "${path_conf_file_path}")
  file(MAKE_DIRECTORY "${path_conf_dir}")
  file(TOUCH "${path_conf_file_path}")
endif()

##############################################################################
# Find Python
##############################################################################
# First search for Python with Interpreter+Devel components, to allow the module
# to look for the interpreter that goes with the Python development library.
# Then if it could not find both, try looking for the development component
# only, but this time force the module to find it or fail (REQUIRED).
# Some of our toolchains (when crosscompiling for instance) have the Python
# library but not an interpreter that can run on the host.
find_package(Python COMPONENTS Development Interpreter)
if(NOT Python_FOUND)
  find_package(Python REQUIRED COMPONENTS Development)
endif()

##############################################################################
# Find Boost
##############################################################################
find_package(
  Boost REQUIRED
  COMPONENTS thread
)

##############################################################################
# Find Pybind11
##############################################################################
find_package(pybind11 REQUIRED)

##############################################################################
# Find LibQi
##############################################################################
find_package(qi REQUIRED)

##############################################################################
# Convenience library: cxx_standard
##############################################################################
add_library(cxx_standard INTERFACE)

# The project requires at least C++17.
target_compile_features(
  cxx_standard
  INTERFACE
    cxx_std_17
)

##############################################################################
# Library: qi_python_objects
##############################################################################
add_library(qi_python_objects OBJECT)

target_sources(
  qi_python_objects

  PUBLIC
    qipython/common.hpp
    qipython/pyapplication.hpp
    qipython/pyasync.hpp
    qipython/pyclock.hpp
    qipython/pyexport.hpp
    qipython/pyfuture.hpp
    qipython/pylog.hpp
    qipython/pymodule.hpp
    qipython/pyobject.hpp
    qipython/pypath.hpp
    qipython/pyproperty.hpp
    qipython/pysession.hpp
    qipython/pysignal.hpp
    qipython/pyguard.hpp
    qipython/pytranslator.hpp
    qipython/pytypes.hpp
    qipython/pystrand.hpp

  PRIVATE
    src/pyapplication.cpp
    src/pyasync.cpp
    src/pyclock.cpp
    src/pyexport.cpp
    src/pyfuture.cpp
    src/pylog.cpp
    src/pymodule.cpp
    src/pyobject.cpp
    src/pypath.cpp
    src/pyproperty.cpp
    src/pysession.cpp
    src/pysignal.cpp
    src/pystrand.cpp
    src/pytranslator.cpp
    src/pytypes.cpp
)

target_include_directories(
  qi_python_objects
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
  qi_python_objects
  PUBLIC
    cxx_standard
    Boost::headers
    Boost::thread
    pybind11::pybind11
    qi::qi
)

set_target_properties(
  qi_python_objects
  PROPERTIES
    CXX_EXTENSIONS TRUE
    CXX_VISIBILITY_PRESET hidden
    # Use PIC as the library is linked into a shared library in the qi_python
    # target.
    POSITION_INDEPENDENT_CODE TRUE
)

##############################################################################
# Library: qi_python
##############################################################################
pybind11_add_module(
  qi_python
  # Disable generation of an extension for the shared library.
  WITHOUT_SOABI
)

# Generate a Python file containing the version.
set(version_py_file "${CMAKE_CURRENT_BINARY_DIR}/qi/_version.py")
configure_file(qi/_version.py.in "${version_py_file}" @ONLY)

# Generate a Python file containing information about the native part of the
# module.
set(native_py_file "${CMAKE_CURRENT_BINARY_DIR}/qi/native.py")
if(qi_VERSION)
    set(NATIVE_VERSION "${qi_VERSION}")
else()
    set(NATIVE_VERSION "unknown")
endif()
configure_file(qi/native.py.in "${native_py_file}" @ONLY)

set(
  qi_python_py_files
    qi/__init__.py
    qi/logging.py
    qi/path.py
    qi/translator.py
    qi/_binder.py
    qi/_type.py
)
set(
  qi_python_test_py_files
    qi/test/__init__.py
    qi/test/conftest.py
    qi/test/test_applicationsession.py
    qi/test/test_async.py
    qi/test/test_call.py
    qi/test/test_log.py
    qi/test/test_module.py
    qi/test/test_promise.py
    qi/test/test_property.py
    qi/test/test_return_empty_object.py
    qi/test/test_session.py
    qi/test/test_signal.py
    qi/test/test_strand.py
    qi/test/test_typespassing.py
)

target_sources(
  qi_python
  PUBLIC
    ${qi_python_py_files}
  PRIVATE
    src/module.cpp
    ${qi_python_test_py_files}
)

target_link_libraries(
  qi_python
  PRIVATE
    cxx_standard
    qi_python_objects
)

set_target_properties(
  qi_python
  PROPERTIES
    CXX_EXTENSIONS TRUE
    CXX_VISIBILITY_PRESET hidden
    LIBRARY_OUTPUT_DIRECTORY qi
    RUNTIME_OUTPUT_DIRECTORY qi
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

##############################################################################
# Library: qipython_module_plugin
##############################################################################
add_library(qimodule_python_plugin SHARED)
target_sources(
  qimodule_python_plugin
  PRIVATE
    src/qimodule_python_plugin.cpp
)
target_link_libraries(
  qimodule_python_plugin
  PRIVATE
    cxx_standard
    qi::qi
)

##############################################################################
# Installation
##############################################################################
install(
  TARGETS qi_python
  LIBRARY DESTINATION qi COMPONENT Module
  # On Windows, shared libraries (DLL) are considered `RUNTIME`.
  RUNTIME DESTINATION qi COMPONENT Module
)

# Install Python files.
install(
  FILES ${qi_python_py_files} "${version_py_file}" "${native_py_file}"
  DESTINATION qi
  COMPONENT Module
)

##############################################################################
# Tests
##############################################################################
if(BUILD_TESTING)
  find_package(GTest REQUIRED)

  # Use the CMake GoogleTest module that offers functions to automatically
  # discover tests.
  include(GoogleTest)

  find_package(qimodule REQUIRED)
  qi_create_module(moduletest NO_INSTALL)
  target_sources(
    moduletest
    PRIVATE
      tests/moduletest.cpp
  )
  target_link_libraries(
    moduletest
    PRIVATE
      cxx_standard
  )

  add_executable(service_object_holder)
  target_sources(
    service_object_holder
    PRIVATE
      tests/service_object_holder.cpp
  )
  target_link_libraries(
    service_object_holder
    PRIVATE
      cxx_standard
      qi::qi
  )

  add_executable(test_qipython)
  target_sources(
    test_qipython
    PRIVATE
      tests/common.hpp
      tests/test_qipython.cpp
      tests/test_guard.cpp
      tests/test_types.cpp
      tests/test_signal.cpp
      tests/test_property.cpp
      tests/test_object.cpp
      tests/test_module.cpp
  )
  target_link_libraries(
    test_qipython
      PRIVATE
        qi_python_objects
        cxx_standard
        Python::Python
        Boost::headers
        pybind11::pybind11
        GTest::gmock
  )
  gtest_discover_tests(test_qipython)

  add_executable(test_qipython_local_interpreter)
  target_sources(
    test_qipython_local_interpreter
    PRIVATE
      tests/test_qipython_local_interpreter.cpp
  )
  target_link_libraries(
    test_qipython_local_interpreter
    PRIVATE
      Python::Python
      pybind11::pybind11
      qi_python_objects
      cxx_standard
      GTest::gmock
  )
  gtest_discover_tests(test_qipython_local_interpreter)

  if(NOT Python_Interpreter_FOUND)
    message(WARNING "tests: a compatible Python Interpreter was NOT found, Python tests are DISABLED.")
  else()
    message(STATUS "tests: a compatible Python Interpreter was found, Python tests are enabled.")

    execute_process(
      COMMAND "${Python_EXECUTABLE}" -m pytest --version
      OUTPUT_QUIET
      ERROR_QUIET
      RESULT_VARIABLE pytest_version_err
    )
    if(pytest_version_err)
      message(WARNING "tests: the pytest module does not seem to be installed for this Python Interpreter, "
                      "the execution of the tests will most likely result in a failure.")
    endif()

    # Copy python files so that tests can be run in the build directory, with the generated files.
    foreach(file IN LISTS qi_python_py_files qi_python_test_py_files)
      configure_file("${file}" "${file}" COPYONLY)
    endforeach()

    add_test(
      NAME pytest
      COMMAND
        Python::Interpreter
          -m pytest
          "${CMAKE_CURRENT_BINARY_DIR}/qi/test"
          --exec_path "$<TARGET_FILE:service_object_holder> --qi-standalone --qi-listen-url tcp://127.0.0.1:0"
    )
    set_tests_properties(
      pytest
      PROPERTIES
        ENVIRONMENT "QI_SDK_PREFIX=${sdk_dir}"
    )
  endif()
else()
  message(STATUS "tests: tests are disabled")
endif()
