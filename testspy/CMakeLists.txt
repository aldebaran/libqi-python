##
## Author(s):
##  - Pierre ROULLON        <proullon@aldebaran-robotics.com>
##
## Copyright (C) 2012 Aldebaran Robotics

if(UNIX AND NOT APPLE)
  # If we link with toolchain python, we must set PYTHONHOME to use .py files
  # from toolchain python and not system.
  # At this point it only works on linux for unknown reasons.
  set(PYTHONHOME "${PYTHON${pyver}_INCLUDE_DIRS}/../..")
elseif(MSVC)
  set(PYTHONHOME "${PYTHON${pyver}_INCLUDE_DIRS}/../../lib/python2.7")
endif()

if(UNIX)
  get_target_property(sdkpydir _qi${pyver} LIBRARY_OUTPUT_DIRECTORY)
elseif(WIN32)
  # set in qibuild, same as DEBUG
  get_target_property(sdkpydir _qi${pyver} RUNTIME_OUTPUT_DIRECTORY_RELEASE)
endif()

qi_create_gtest(
  test_python${pyver}_typepassing

  SRC
  "test_qipython.cpp"
  "test_module.cpp"
  "test_python_typepassing.cpp"

  DEPENDS
  QIPYTHON${pyver}
  ARGUMENTS "${sdkpydir}" "${CMAKE_CURRENT_SOURCE_DIR}/.." "${PYTHONHOME}"
)
