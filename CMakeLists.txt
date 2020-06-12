cmake_minimum_required(VERSION 2.8)
project(libqi-python)
find_package(qibuild)

if(ANDROID)
  return()
endif()

qi_sanitize_compile_flags(HIDDEN_SYMBOLS)
qi_add_optional_package(PYTHON)
qi_add_optional_package(PYTHON3)
qi_add_optional_package(BOOST_PYTHON)
qi_add_optional_package(BOOST_PYTHON3)

add_subdirectory(tests)

# Prefer Python3 when available, otherwise fallback to Python2.
if(WITH_PYTHON3)
  message(STATUS "Python3: libraries found (\"${PYTHON3_LIBRARIES}\")")
else()
  message(STATUS "Python3: libraries not found")
endif()

if(WITH_BOOST_PYTHON3)
  message(STATUS "boost-python3: libraries found (\"${BOOST_PYTHON3_LIBRARIES}\")")
else()
  message(STATUS "boost-python3: libraries no found")
endif()

set(pyver "")
if (WITH_PYTHON3 AND WITH_BOOST_PYTHON3)
  set(pyver "3")
elseif(WITH_PYTHON AND WITH_BOOST_PYTHON)
  message(STATUS "Either Python3 or boost-python3 libraries are \
missing, falling back to building for Python2")
else()
  message(FATAL_ERROR "Neither Python3/boost-python3 nor \
Python2/boost-python libraries have been found")
endif()
add_subdirectory(py2py3 python)


qi_create_lib(qimodule_python_plugin SHARED src/qimodule_python_plugin.cpp
              SUBFOLDER qi/plugins/
              DEPENDS qi qipython${pyver})

if (QI_WITH_TESTS AND WITH_PYTHON AND NOT WIN32 AND NOT APPLE)
  # run py.test.
  # Note: a more generic solution would be to use qipy run & qitest collect
  find_program(_PYTEST_EXECUTABLE py.test REQUIRED)
  # note: surprisingly, the /usr/local/... is needed when pytest in
  # installed under /usr/local.
  qi_add_test(test_run_pytest ${_PYTEST_EXECUTABLE}
      ARGUMENTS "-k" "not return_empty_object"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qi/test"
      ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${QI_SDK_DIR}/lib/python2.7/site-packages:/usr/local/lib/python2.7/dist-packages:$ENV{PYTHONPATH};QI_ADDITIONAL_SDK_PREFIXES=${QI_SDK_DIR}")

  qi_add_test(test_run_pytest_return_empty_object ${_PYTEST_EXECUTABLE}
      ARGUMENTS
        "${CMAKE_CURRENT_SOURCE_DIR}/qi/test/test_return_empty_object.py"
        "--exec_path"
        "${CMAKE_CURRENT_BINARY_DIR}/sdk/bin/service_object_holder --qi-standalone --qi-listen-url tcp://127.0.0.1:0"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/qi/test"
      ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${QI_SDK_DIR}/lib/python2.7/site-packages:/usr/local/lib/python2.7/dist-packages:$ENV{PYTHONPATH};QI_ADDITIONAL_SDK_PREFIXES=${QI_SDK_DIR}")
endif()
