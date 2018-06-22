cmake_minimum_required(VERSION 2.8)
project(libqi-python)
find_package(qibuild)

qi_sanitize_compile_flags(HIDDEN_SYMBOLS)
qi_add_optional_package(PYTHON)
qi_add_optional_package(PYTHON3)
qi_add_optional_package(BOOST_PYTHON)
qi_add_optional_package(BOOST_PYTHON3)
# Generate each version in a separated build folder.
# (include_directories clash otherwise)

add_subdirectory(tests)

if (WITH_PYTHON AND WITH_BOOST_PYTHON)
  set(pyver "")
  add_subdirectory(py2py3 "python2")
endif()

if (WITH_PYTHON3 AND WITH_BOOST_PYTHON3)
  set(pyver "3")
  add_subdirectory(py2py3 "python3")
endif()


qi_create_lib(qimodule_python_plugin SHARED src/qimodule_python_plugin.cpp
              SUBFOLDER qi/plugins/
              DEPENDS qi qipython)

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
