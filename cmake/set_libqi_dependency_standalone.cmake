# In standalone mode, LibQi is currently added as a subproject. It's a
# controversial choice but depending on a prebuilt configuration, installation
# or package is messy with our CMake code. It makes compilation longer but it
# ensures consistency.

include_guard(GLOBAL)

if(NOT COMMAND FetchContent_Declare)
  include(FetchContent)
endif()

if(FETCHCONTENT_SOURCE_DIR_LIBQI)
  message(STATUS "LibQi: using sources at \"${FETCHCONTENT_SOURCE_DIR_LIBQI}\"")
else()
  message(STATUS "LibQi: using sources from \"${LIBQI_GIT_REPOSITORY}\" at tag \
\"${LIBQI_GIT_TAG}\"")
endif()

FetchContent_Declare(LibQi
  GIT_REPOSITORY "${LIBQI_GIT_REPOSITORY}"
  GIT_TAG        "${LIBQI_GIT_TAG}"
)
FetchContent_GetProperties(LibQi)
if(NOT libqi_POPULATED)
  FetchContent_Populate(LibQi)
endif()

check_libqi_version("${libqi_SOURCE_DIR}/package.xml" LIBQI_PACKAGE_VERSION)

# Adds LibQi as a subproject which will create a `qi` target.
#
# We use a function in order to scope the value of a few variables only for the
# LibQi subproject.
function(add_libqi_subproject)
  # Do not build examples and tests, we don't need them in this project.
  set(BUILD_EXAMPLES OFF)
  set(QI_WITH_TESTS OFF)

  # Add it as a subproject but use `EXCLUDE_FROM_ALL` to disable the installation
  # rules of the subproject files.com
  add_subdirectory(${libqi_SOURCE_DIR} ${libqi_BINARY_DIR} EXCLUDE_FROM_ALL)
endfunction()
add_libqi_subproject()

add_library(qi.interface INTERFACE)

# Add the subproject include directories as "system" to avoid warnings
# generated by LibQi headers.
target_include_directories(qi.interface SYSTEM INTERFACE
                           ${libqi_SOURCE_DIR}
                           ${libqi_BINARY_DIR}/include)

target_link_libraries(qi.interface INTERFACE
                      qi

# qibuild CMake macros fail to propagate LibQi's include directories to the
# interface of the `qi` target, so we have to add them ourselves. For Boost,
# adding the header-only target is sufficient, as it will add the include
# directories, and the link libraries are already part of the `qi` target's
# interface (qibuild propagates those fine).
                      Boost::boost

# These targets are not strictly speaking libraries but they add flags that
# disable autolinking in Boost and force dynamic linking, which are required
# on Windows.
                      Boost::disable_autolinking
                      Boost::dynamic_linking

# For the libraries other than Boost, we don't have a header-only target to
# only add the include directories, so we have to directly link to them.
                      OpenSSL::SSL OpenSSL::Crypto)