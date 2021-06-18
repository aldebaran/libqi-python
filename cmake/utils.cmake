include_guard(GLOBAL)

# Parses a Python version as defined in PEP440
# (see https://www.python.org/dev/peps/pep-0440/) in some input and sets the
# following variables accordingly:
#
#   - `<prefix>_VERSION_STRING`, the whole version string
#   - `<prefix>_VERSION_EPOCH`, the epoch part of the version
#   - `<prefix>_VERSION_RELEASE`, the release part of the version, itself
#      splitted into:
#      - `<prefix>_VERSION_RELEASE_MAJOR`
#      - `<prefix>_VERSION_RELEASE_MINOR`
#      - `<prefix>_VERSION_RELEASE_MAJOR_MINOR` equivalent to "<major>.<minor>".
#      - `<prefix>_VERSION_RELEASE_PATCH`
#   - `<prefix>_VERSION_PRERELEASE`
#   - `<prefix>_VERSION_POSTRELEASE`
#   - `<prefix>_VERSION_DEVRELEASE`
function(parse_python_version prefix input)
  if(NOT input)
    return()
  endif()

  # Simplified regex
  set(_regex "^([0-9]+!)?([0-9]+)((\\.[0-9]+)*)([abrc]+[0-9]+)?(\\.(post[0-9]+))?(\\.(dev[0-9]+))?$")
  string(REGEX MATCH "${_regex}" _match "${input}")
  if(NOT _match)
    message(WARNING "Could not parse a Python version from '${input}'.")
    return()
  endif()

  set(${prefix}_VERSION_STRING "${_match}" PARENT_SCOPE)
  set(${prefix}_VERSION_EPOCH "${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(_release "${CMAKE_MATCH_2}${CMAKE_MATCH_3}")
  set(${prefix}_VERSION_RELEASE ${_release} PARENT_SCOPE)
  set(${prefix}_VERSION_PRERELEASE "${CMAKE_MATCH_5}" PARENT_SCOPE)
  set(${prefix}_VERSION_POSTRELEASE "${CMAKE_MATCH_7}" PARENT_SCOPE)
  set(${prefix}_VERSION_DEVRELEASE "${CMAKE_MATCH_9}" PARENT_SCOPE)

  set(_release_regex "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?$")
  string(REGEX MATCH "${_release_regex}" _match "${_release}")
  if(NOT _match)
    message(WARNING "Could not parse a Python release version from '${_release}'.")
    return()
  endif()

  set(${prefix}_VERSION_RELEASE_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
  set(${prefix}_VERSION_RELEASE_MINOR "${CMAKE_MATCH_3}" PARENT_SCOPE)
  set(${prefix}_VERSION_RELEASE_MAJOR_MINOR "${CMAKE_MATCH_1}${CMAKE_MATCH_2}" PARENT_SCOPE)
  set(${prefix}_VERSION_RELEASE_PATCH "${CMAKE_MATCH_5}" PARENT_SCOPE)
endfunction()

# Enables most compiler warnings for a target.
function(enable_warnings target)
  if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  elseif(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  endif()
endfunction()

# Sets the install RPATH of some targets to a list of paths relative to the
# location of the binary.
function(set_install_rpath)
  cmake_parse_arguments(SET_INSTALL_RPATH "ORIGIN" "" "TARGETS;PATHS_REL_TO_ORIGIN" ${ARGN})
  foreach(_target IN LISTS SET_INSTALL_RPATH_TARGETS)
    set(_paths)
    if(SET_INSTALL_RPATH_ORIGIN)
      message(VERBOSE "Adding $ORIGIN to RPATH of ${_target}")
      list(APPEND _paths
        "$<$<PLATFORM_ID:Linux>:$ORIGIN>$<$<PLATFORM_ID:Darwin>:@loader_path>")
    endif()
    foreach(_path IN LISTS SET_INSTALL_RPATH_PATHS_REL_TO_ORIGIN)
      if(NOT _path)
        continue()
      endif()
      message(VERBOSE "Adding $ORIGIN/${_path} to RPATH of ${_target}")
      list(APPEND _paths
        "$<$<PLATFORM_ID:Linux>:$ORIGIN/${_path}>$<$<PLATFORM_ID:Darwin>:@loader_path/${_path}>")
    endforeach()
    if(_paths)
      message(VERBOSE "Setting ${_target} RPATH to ${_paths}")
      set_property(TARGET ${_target} PROPERTY INSTALL_RPATH ${_paths})
    endif()
  endforeach()
endfunction()

# Creates a variable that can be overridden by the user from either the
# command-line, the cache or the environment.
# The order of preference is:
#   - the value from the variable in cache (or the command-line, since setting
#   a variable from the command-line automatically adds it to the cache).
#   - the value from the variable in the environment.
#   - the default value for the variable.
function(overridable_variable name default_value)
  # The variable already exists in the cache. It's the preferred value, so we
  # don't change it.
  if(DEFINED CACHE{${name}})
    return()
  endif()

  set(value ${default_value})
  if(DEFINED ENV{${name}})
    set(value $ENV{${name}})
  endif()
  set(${name} ${value} PARENT_SCOPE)
endfunction()
