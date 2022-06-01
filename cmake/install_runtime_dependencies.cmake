set(_module_install_dir
    "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${QIPYTHON_PYTHON_MODULE_NAME}")

# On Windows, shared libraries (DLL) are not in the `lib` subdirectory of
# the dependencies, but in the `bin` directory. So for each dependency directory
# that ends with `/lib`, we add the corresponding `/bin` directory.
set(_runtime_dependencies_dirs ${QIPYTHON_BUILD_DEPENDENCIES_LIBRARY_DIRS})
if(WIN32)
  set(_dep_dirs ${_runtime_dependencies_dirs})
  foreach(_dep_dir IN LISTS _dep_dirs)
    if(_dep_dir MATCHES "/lib$")
      get_filename_component(_full_dep_bin_dir "${_dep_dir}/../bin" ABSOLUTE)
      list(APPEND _runtime_dependencies_dirs "${_full_dep_bin_dir}")
    endif()
  endforeach()
endif()

# Install the runtime dependencies of the module, eventually by copying the
# libraries from the toolchain.
message(STATUS "Looking for dependencies of qi_python and qi in: ${_runtime_dependencies_dirs}")
file(GET_RUNTIME_DEPENDENCIES
     LIBRARIES "${QIPYTHON_QI_TARGET_FILE}"
     MODULES "${QIPYTHON_QI_PYTHON_TARGET_FILE}"
     RESOLVED_DEPENDENCIES_VAR _runtime_dependencies
     UNRESOLVED_DEPENDENCIES_VAR _unresolved_runtime_dependencies
     CONFLICTING_DEPENDENCIES_PREFIX _conflicting_runtime_dependencies
     # eay32 is a library that comes with OpenSSL.
     PRE_INCLUDE_REGEXES ssl eay crypto icu boost
     PRE_EXCLUDE_REGEXES .*
     DIRECTORIES ${_runtime_dependencies_dirs})

# Processing potential conflicts in dependencies: if there are multiple
# available choices for one of the runtime dependencies, we prefer one
# that is in the toolchain. Otherwise we just fallback to the first
# available path for that dependency.
if(_conflicting_runtime_dependencies_FILENAMES)
  message(STATUS "Some conflicts were found for dependencies of qi_python and qi: ${_conflicting_runtime_dependencies_FILENAMES}")
  foreach(_filename IN LISTS _conflicting_runtime_dependencies_FILENAMES)
    message(STATUS "Conflicts of dependency '${_filename}': ${_conflicting_runtime_dependencies_${_filename}}")
    set(_dep)
    foreach(_path IN LISTS _conflicting_runtime_dependencies_${_filename})
      if(QIPYTHON_TOOLCHAIN_DIR AND _path MATCHES "^${QIPYTHON_TOOLCHAIN_DIR}")
        set(_dep "${_path}")
        message(STATUS "Using file '${_dep}' for dependency '${_filename}' as it is in the toolchain.")
        break()
      endif()
    endforeach()
    if(NOT _dep)
      list(GET _conflicting_runtime_dependencies_${_filename} 0 _dep)
      message(STATUS "Fallback to the first available path '${_dep}' for dependency '${_filename}'.")
    endif()
    list(APPEND _runtime_dependencies "${_dep}")
  endforeach()
endif()

message(STATUS "The dependencies of qi_python and qi are: ${_runtime_dependencies}")
if(_unresolved_runtime_dependencies)
  message(STATUS "Some dependencies of qi_python and qi are unresolved: ${_unresolved_runtime_dependencies}")
endif()

if(UNIX OR APPLE)
  find_program(_patchelf "patchelf")
  if(_patchelf)
    if(APPLE)
      set(_patchelf_runtime_path "@loader_path")
    elseif(UNIX)
      set(_patchelf_runtime_path "$ORIGIN")
    endif()
  else()
    message(WARNING "The target system seems to be using ELF binaries, but the `patchelf` \
tool could not be found. The installed runtime dependencies might not have their runtime path \
set correctly to run. You might want to install `patchelf` on your system.")
  endif()
endif()

foreach(_needed_dep IN LISTS _runtime_dependencies)
  set(_dep "${_needed_dep}")

  # Some of the dependency libraries are symbolic links, and `file(INSTALL)` will
  # copy the symbolic link instead of their target (the real library) by default.
  # There is an option FOLLOW_SYMLINK_CHAIN that will copy both the library and
  # the symlink, but then when we create an archive (a wheel for instance),
  # symlinks are replaced by the file they point to. This results in libraries
  # being copied multiple times in the same archive with different names.
  #
  # Therefore we have to detect symlink manually and copy the files ourselves.
  while(IS_SYMLINK "${_dep}")
    set(_symlink "${_dep}")
    file(READ_SYMLINK "${_symlink}" _dep)
     if(NOT IS_ABSOLUTE "${_dep}")
      get_filename_component(_dir "${_symlink}" DIRECTORY)
      set(_dep "${_dir}/${_dep}")
    endif()
    message(STATUS "Dependency ${_symlink} is a symbolic link, resolved to ${_dep}")
  endwhile()

  file(INSTALL "${_dep}" DESTINATION "${_module_install_dir}")
  get_filename_component(_dep_filename "${_dep}" NAME)
  set(_installed_dep "${_module_install_dir}/${_dep_filename}")

  if(_patchelf)
    message(STATUS "Set runtime path of runtime dependency \"${_installed_dep}\" to \"${_patchelf_runtime_path}\"")
    file(TO_NATIVE_PATH "${_installed_dep}" _native_installed_dep)
    execute_process(COMMAND "${_patchelf}"
      # Sets the RPATH/RUNPATH or may replace the RPATH by a RUNPATH, depending on the platform.
      --set-rpath "${_patchelf_runtime_path}" "${_native_installed_dep}")
  endif()

  if(NOT "${_dep}" STREQUAL "${_needed_dep}")
    get_filename_component(_needed_dep_filename "${_needed_dep}" NAME)
    set(_installed_needed_dep "${_module_install_dir}/${_needed_dep_filename}")
    message(STATUS "Renaming ${_installed_dep} into ${_installed_needed_dep}")
    file(RENAME "${_installed_dep}" "${_installed_needed_dep}")
    list(TRANSFORM CMAKE_INSTALL_MANIFEST_FILES
         REPLACE "${_installed_dep}" "${_installed_needed_dep}")
  endif()
endforeach()
