#!/bin/sh
set -x -e

PACKAGE=$1

pip install 'conan>=2' 'cmake>=3.23' ninja

# Perl dependencies required to build OpenSSL.
yum install -y perl-IPC-Cmd perl-Digest-SHA

# Install Conan configuration.
conan config install "$PACKAGE/ci/conan"

# Clone and export libqi to Conan cache.
# Possible improvement:
#   Avoid duplicating the version number here with the version
#   defined in conanfile.py.
GIT_SSL_NO_VERIFY=true \
    git clone --depth=1 \
        --branch qi-framework-v4.0.1 \
        "$LIBQI_REPOSITORY_URL" \
        /work/libqi
conan export /work/libqi --version=4.0.1

# Install dependencies of libqi-python from Conan, including libqi.
# Only use the build_type as a variable for the build folder name, so
# that the generated CMake preset is named "conan-release".
#
# Build everything from sources, so that we do not reuse precompiled binaries.
# This is because the GLIBC from the manylinux images are often older than the
# ones that were used to build the precompiled binaries, which means the binaries
# cannot by executed.
conan install "$PACKAGE" \
    --build="*" \
    -c tools.build:skip_test=true \
    -c tools.cmake.cmaketoolchain:generator=Ninja \
    -c tools.cmake.cmake_layout:build_folder_vars=[\"settings.build_type\"]
