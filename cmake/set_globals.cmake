include_guard(GLOBAL)

# Convert relative paths in `target_sources` to absolute.
cmake_policy(SET CMP0076 NEW)

# Set the default policies for subprojects using an older CMake version
# (specified through `cmake_minimum_required`). Setting the policy directly
# (through `cmake_policy`) is not enough as it will be overwritten by the call
# to `cmake_minimum_required` in the subproject. This is to remove spurious
# warnings we get otherwise.
#
# 0048 - project() command manages VERSION variables.
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
# 0056 - Honor link flags in try_compile() source-file signature.
set(CMAKE_POLICY_DEFAULT_CMP0056 NEW)
# 0066 - Honor per-config flags in try_compile() source-file signature.
set(CMAKE_POLICY_DEFAULT_CMP0066 NEW)
# 0060 - Link libraries by full path even in implicit directories.
set(CMAKE_POLICY_DEFAULT_CMP0060 NEW)
# 0063 - Honor visibility properties for all target types.
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
# 0074 - find_package uses PackageName_ROOT variables.
set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)
# 0077 - option() honors normal variables.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
# 0082 - Install rules from add_subdirectory() are interleaved with those in caller.
set(CMAKE_POLICY_DEFAULT_CMP0082 NEW)

# Do not export symbols by default on new targets.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# We use RPATH on systems that handle them.
set(CMAKE_SKIP_RPATH OFF)
set(CMAKE_SKIP_BUILD_RPATH OFF)

# Do not use the installation RPATH/RUNPATH values for the build directory.
# Our build directories do not necessarily share the same structure as our install
# directories.
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

# Do not append to the runtime search path (rpath) of installed binaries any
# directory outside the project that is in the linker search path or contains
# linked library files.
# Instead we copy the dependencies we need in the install directory, and we set
# the right RPATH ourselves.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
