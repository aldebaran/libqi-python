# In system mode, LibQi must be accessible through find_package.
find_package(qi REQUIRED CONFIG)

# If the qi package was found from a build directory in LibQi sources (which
# is the case when built with qibuild), then we expect that `qi_DIR` is set
# to the path `<...>/libqi/build/sdk/cmake`.
# If it was found in one of our toolchains, then we expect that `qi_DIR` is
# set to the path `<toolchain>/libqi(or qi-framework)/share/cmake/qi` in which
# case the `package.xml` file is in `<toolchain>/libqi(or qi-framework)`.
get_filename_component(QIPYTHON_LIBQI_PACKAGE_FILE
                       "${qi_DIR}/../../../package.xml" ABSOLUTE)
if(NOT EXISTS "${QIPYTHON_LIBQI_PACKAGE_FILE}")
  get_filename_component(QIPYTHON_LIBQI_PACKAGE_FILE
                         "${qi_DIR}/../../package.xml" ABSOLUTE)
endif()

set(QIPYTHON_LIBQI_PACKAGE_FILE "${QIPYTHON_LIBQI_PACKAGE_FILE}" CACHE FILEPATH
    "Path to the `package.xml` file of LibQi.")

if(LIBQI_VERSION)
  if(EXISTS "${QIPYTHON_LIBQI_PACKAGE_FILE}")
    check_libqi_version("${QIPYTHON_LIBQI_PACKAGE_FILE}" LIBQI_PACKAGE_VERSION)
  else()
    message(WARNING "The `package.xml` file for LibQi could not be found at \
'${QIPYTHON_LIBQI_PACKAGE_FILE}' (the LibQi package was found in '${qi_DIR}'). \
The version of LibQi could not be verified. Please manually specify a valid \
path as the 'QIPYTHON_LIBQI_PACKAGE_FILE' variable to fix this issue, or unset \
the 'LIBQI_VERSION' variable to completely disable this check.")
  endif()
endif()

add_library(qi.interface INTERFACE)
target_include_directories(qi.interface SYSTEM INTERFACE ${QI_INCLUDE_DIRS})
target_link_libraries(qi.interface INTERFACE ${QI_LIBRARIES})

foreach(_dep IN LISTS QI_DEPENDS)
  if(NOT ${_dep}_PACKAGE_FOUND)
    find_package(${_dep} REQUIRED CONFIG)
  endif()
  target_include_directories(qi.interface SYSTEM INTERFACE ${${_dep}_INCLUDE_DIRS})
  target_link_libraries(qi.interface INTERFACE ${${_dep}_LIBRARIES})
endforeach()
