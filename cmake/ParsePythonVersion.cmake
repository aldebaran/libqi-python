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
