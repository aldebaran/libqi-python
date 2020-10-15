include_guard(GLOBAL)


if(NOT Python_VERSION_MAJOR OR NOT Python_VERSION_MINOR)
  message(AUTHOR_WARNING "The Python major or minor version is unknown.")
endif()
set(_sitelib_dir
    "lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/${QIPYTHON_PYTHON_MODULE_NAME}")

# Set the RPATH of the module to a relative libraries directory. We assume that,
# if the module is installed in "/path/lib/pythonX.Y/site-packages/qi", the
# libraries we need are instead in "/path/lib", so we set the RPATH accordingly.
set_install_rpath(TARGETS qi_python ORIGIN PATHS_REL_TO_ORIGIN "../../..")

install(TARGETS qi_python
        LIBRARY DESTINATION "${_sitelib_dir}"
        COMPONENT runtime)

install(FILES ${QIPYTHON_PYTHON_MODULE_FILES}
        DESTINATION "${_sitelib_dir}"
        COMPONENT runtime)

# Install the Python file containing native information.
install(FILES "${QIPYTHON_NATIVE_PYTHON_FILE}"
        DESTINATION "${_sitelib_dir}"
        COMPONENT runtime)

# Set the RPATH of the plugin to a relative libraries directory. We assume that,
# if the plugin is installed in "/path/lib/qi/plugins", the
# libraries we need are instead in "/path/lib", so we set the RPATH accordingly.
set_install_rpath(TARGETS qimodule_python_plugin ORIGIN PATHS_REL_TO_ORIGIN "../..")

install(TARGETS qimodule_python_plugin
        LIBRARY DESTINATION "lib/qi/plugins"
        COMPONENT runtime)