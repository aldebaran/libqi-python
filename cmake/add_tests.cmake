include_guard(GLOBAL)

if(NOT QI_WITH_TESTS)
  message(STATUS "tests: tests are disabled")
  return()
endif()

if(CMAKE_CROSSCOMPILING)
  message(STATUS "tests: crosscompiling, tests are disabled")
  return()
endif()

enable_testing()

# Use the CMake GoogleTest module that offers functions to automatically discover
# tests.
include(GoogleTest)


find_package(qimodule REQUIRED HINTS ${libqi_SOURCE_DIR})
qi_create_module(moduletest NO_INSTALL)
target_sources(moduletest PRIVATE tests/moduletest.cpp)
target_link_libraries(moduletest cxx11 qi.interface)
enable_warnings(moduletest)
set_build_rpath_to_qipython_dependencies(moduletest)


add_executable(service_object_holder)
target_sources(service_object_holder PRIVATE tests/service_object_holder.cpp)
target_link_libraries(service_object_holder PRIVATE cxx11 qi.interface)
enable_warnings(service_object_holder)
set_build_rpath_to_qipython_dependencies(service_object_holder)


add_executable(test_qipython)
target_sources(test_qipython
  PRIVATE tests/common.hpp
          tests/test_qipython.cpp
          tests/test_guard.cpp
          tests/test_types.cpp
          tests/test_signal.cpp
          tests/test_property.cpp
          tests/test_object.cpp
          tests/test_module.cpp)
target_link_libraries(test_qipython
  PRIVATE Python::Python
          pybind11
          qi_python_objects
          qi.interface
          cxx11
          gmock)

add_executable(test_qipython_local_interpreter)
target_sources(test_qipython_local_interpreter
  PRIVATE tests/test_qipython_local_interpreter.cpp)
target_link_libraries(test_qipython_local_interpreter
  PRIVATE Python::Python
          pybind11
          qi_python_objects
          qi.interface
          cxx11
          gmock)

set(_sdk_prefix "${CMAKE_BINARY_DIR}/sdk")
foreach(test_target IN ITEMS test_qipython
                             test_qipython_local_interpreter)
  # Unfortunately, in some of our toolchains, gtest/gmock headers are found in the qi-framework
  # package, which comes first in the include paths order given to the compiler. This causes the
  # compiler to use those headers instead of the ones we got from a `FetchContent` of the googletest
  # repository.
  target_include_directories(${test_target}
    BEFORE # Force this path to come first in the list of include paths.
    PRIVATE $<TARGET_PROPERTY:gmock,INTERFACE_INCLUDE_DIRECTORIES>)
  enable_warnings(${test_target})
  set_build_rpath_to_qipython_dependencies(${test_target})
  gtest_discover_tests(${test_target} EXTRA_ARGS --qi-sdk-prefix ${_sdk_prefix})
endforeach()

if(NOT Python_Interpreter_FOUND)
  message(WARNING "tests: a compatible Python Interpreter was NOT found, Python tests are DISABLED.")
else()
  message(STATUS "tests: a compatible Python Interpreter was found, Python tests are enabled.")

  # qibuild sets these variables which tends to mess up our calls of the Python interpreter, so
  # we reset them preemptively.
  set(ENV{PYTHONHOME})
  set(ENV{PYTHONPATH})

  execute_process(COMMAND "${Python_EXECUTABLE}" -m pytest --version
                  OUTPUT_QUIET ERROR_QUIET
                  RESULT_VARIABLE pytest_version_err)
  if(pytest_version_err)
    message(WARNING "tests: the pytest module does not seem to be installed for this Python Interpreter\
, the execution of the tests will most likely result in a failure.")
  endif()

  macro(copy_py_files dir)
    file(GLOB _files RELATIVE "${PROJECT_SOURCE_DIR}" CONFIGURE_DEPENDS "${dir}/*.py")
    foreach(_file IN LISTS _files)
      set_property(DIRECTORY APPEND PROPERTY
                   CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/${_file}")
      file(COPY "${_file}" DESTINATION "${dir}")
    endforeach()
  endmacro()

  copy_py_files(qi)
  copy_py_files(qi/test)

  add_test(NAME pytest
    COMMAND Python::Interpreter -m pytest qi/test
              --exec_path
              "$<TARGET_FILE:service_object_holder> --qi-standalone --qi-listen-url tcp://127.0.0.1:0")

  get_filename_component(_ssl_dir "${OPENSSL_SSL_LIBRARY}" DIRECTORY)
  set(_pytest_env
    "QI_SDK_PREFIX=${_sdk_prefix}"
    "LD_LIBRARY_PATH=${_ssl_dir}")
  set_tests_properties(pytest PROPERTIES ENVIRONMENT "${_pytest_env}")
endif()

# Ensure compatibility with qitest by simply running ctest. Timeout is set to 4 minutes.
file(WRITE
    "${CMAKE_BINARY_DIR}/qitest.cmake" "--name;run_ctest;--timeout;240;--working-directory;\
${CMAKE_CURRENT_BINARY_DIR};--env;PYTHONHOME=;--env;PYTHONPATH=;--;${CMAKE_CTEST_COMMAND};-V")
