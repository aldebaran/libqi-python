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
