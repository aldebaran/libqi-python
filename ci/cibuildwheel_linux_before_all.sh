#!/bin/sh
set -x -e

PACKAGE=$1

pip install 'conan>=2' 'cmake>=3.23' ninja

# Perl dependencies required to build OpenSSL.
yum install -y perl-IPC-Cmd perl-Digest-SHA perl-Time-Piece

# Install Conan configuration.
conan profile detect
conan config install "$PACKAGE/ci/conan"

# Clone and export libqi to Conan cache.
GIT_SSL_NO_VERIFY=true \
    git clone \
        --branch master \
        https://github.com/aldebaran/libqi.git \
        /work/libqi
conan export /work/libqi

# Install dependencies of libqi-python from Conan, including libqi.
#
# Build everything from sources, so that we do not reuse precompiled binaries.
# This is because the GLIBC from the manylinux images are often older than the
# ones that were used to build the precompiled binaries, which means the binaries
# cannot by executed.
#
# Use a fixed output folder so that the generated toolchain file path is
# predictable and can be referenced by scikit-build-core.
conan install "$PACKAGE" --build="*" --profile:all default --profile:all cppstd17 \
    --output-folder=/conan-generators
