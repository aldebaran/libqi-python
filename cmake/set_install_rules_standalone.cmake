include_guard(GLOBAL)

if(NOT QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS)
  message(AUTHOR_WARNING "The QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS is empty.")
endif()

# Stripping the binaries enables us to have lighter wheels.
#
# CMake offers a target "install/strip" that normally does that automatically,
# but scikit-build uses the plain "install" target and this behavior is not
# easily customisable. So this option exists so that we may manually strip the
# binaries when set.
option(QIPYTHON_FORCE_STRIP
       "If set, forces the stripping of the qi_python module at install."
       TRUE)

if(${QIPYTHON_FORCE_STRIP})
  message(STATUS "The qi_python library will be stripped at install.")
  install(CODE "set(CMAKE_INSTALL_DO_STRIP TRUE)"
          COMPONENT Module)
endif()

# qibuild automatically installs a `path.conf` file that we don't really care
# about in our wheel, and we have no other way to disable its installation other
# than letting it be installed then removing it.
install(CODE
  "set(_path_conf \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/qi/path.conf\")
  file(REMOVE \${_path_conf})
  list(REMOVE_ITEM CMAKE_INSTALL_MANIFEST_FILES \${_path_conf})"
  COMPONENT runtime)

if(QIPYTHON_TOOLCHAIN_DIR)
  install(CODE "set(QIPYTHON_TOOLCHAIN_DIR \"${QIPYTHON_TOOLCHAIN_DIR}\")"
          COMPONENT Module)
endif()

install(CODE
  "set(QIPYTHON_PYTHON_MODULE_NAME \"${QIPYTHON_PYTHON_MODULE_NAME}\")
  set(QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS \"${QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS}\")
  set(QIPYTHON_QI_PYTHON_TARGET_FILE \"$<TARGET_FILE:qi_python>\")
  set(QIPYTHON_QI_TARGET_FILE \"$<TARGET_FILE:qi>\")
  set(QIPYTHON_QI_PYTHON_TARGET_FILE_NAME \"$<TARGET_FILE_NAME:qi_python>\")
  set(QIPYTHON_QI_TARGET_FILE_NAME \"$<TARGET_FILE_NAME:qi>\")"
  COMPONENT Module)
install(SCRIPT cmake/install_runtime_dependencies.cmake COMPONENT Module)

set_install_rpath(TARGETS qi_python qi ORIGIN)
install(TARGETS qi_python qi
        LIBRARY DESTINATION "${QIPYTHON_PYTHON_MODULE_NAME}"
        # On Windows, shared libraries (DLL) are considered `RUNTIME`.
        RUNTIME DESTINATION "${QIPYTHON_PYTHON_MODULE_NAME}"
        COMPONENT Module)
