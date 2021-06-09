===================================
LibQiPython - LibQi Python bindings
===================================

This repository contains the official Python bindings of the `LibQi`__, the ``qi``
Python module.

__ LibQi_repo_

Building
========

This project supports the building of a *standalone* package (for instance as a
wheel that can be uploaded on PyPi_) or of a "system" archive.

.. _standalone:

Standalone (wheel)
------------------

This build mode is also referred to as the *standalone* mode. It is enabled by
passing ``-DQIPYTHON_STANDALONE=ON`` to the CMake call. The Python setup script
also sets this mode automatically when used.

In this mode, the project will build libqi and install all its dependencies as
part of the project.

The package can be built from the ``setup.py`` script:

.. code:: bash

  python3 ./setup.py bdist_wheel

or

.. code:: bash

  pip3 wheel . --no-use-pep517

The use of PEP517 is not yet supported and still problematic with our setup
script, so we have to disable it.

The setup script uses scikit-build_ which is itself based on setuptools_. It
handles any option the latter can handle. Additionally, it can take CMake
arguments, which means that you can almost entirely customize how the native
part is built.

In particular, you can use the ``CMAKE_TOOLCHAIN_FILE`` variable to specify a
toolchain to build the native part of the wheel (e.g. if you are using qi
toolchains):

.. code:: bash

  python3 ./setup.py bdist_wheel -DCMAKE_TOOLCHAIN_FILE=$HOME/.local/share/qi/toolchains/my_toolchain/toolchain-my_toolchain.cmake

System archive
--------------

This build mode is also referred to as the "system" mode. This is the default
mode.

In this mode, CMake will expect libqi to be available as a binary package. The
simplest way to build the package in this mode is to use qibuild, which will
first build libqi then libqi-python.

.. code:: bash

  qibuild configure
  qibuild make

You can also set the ``QI_DIR`` variable at the CMake call to let it know it of
the location of the libqi package.

.. code:: bash

  mkdir build && cd build
  cmake .. -DQI_DIR=/path/to/libqi/install/dir
  cmake --build .


Technical details
-----------------

The compiled/native part of this project is based on CMake. It can be built as
any other project of the sort and also through qibuild_, although it requires at
least CMake v3.17.

Our CMake scripts may take a few parameters:

- ``QIPYTHON_STANDALONE``, when set, builds the library in *standalone* mode.
  Refer to the standalone_ section for details.
- ``QIPYTHON_FORCE_STRIP``, when set, forces the build system to strip the
  libqi-python native module library at install, resulting in a smaller binary.
- ``QI_WITH_TESTS``, when set, enables building of tests. This option is ignored
  when cross-compiling.

Dependencies
~~~~~~~~~~~~

The project has a few dependencies and the build system might report errors if
it fails to find them. It uses either ``FindXXX`` CMake modules through the
``find_package`` command or the ``FetchContent`` module for subprojects. Either way,
any parameter that these modules and the ``find_package`` command accept can be
used to customize how the build system finds the libraries or fetches the
subprojects.

Most of the variables described here are defined in the
``cmake/set_dependencies.cmake`` file. You may refer to this file for more details
on these variables and their values.

LibQi
>>>>>

The project's dependencies on LibQi depends on the building mode:

- In **system mode**, it will expect to find it as a binary package. The location
  of the binary package installation can be specified through the ``QI_DIR``
  variable.
- In **standalone mode**, it will download and compile it as a subproject through
  the ``FetchContent`` CMake module. How it is downloaded can be customized
  through the following variables:

  - ``LIBQI_VERSION``
  - ``LIBQI_GIT_REPOSITORY``
  - ``LIBQI_GIT_TAG``

It is possible to skip the download step and use an existing source directory by
setting its path as the ``FETCHCONTENT_SOURCE_DIR_LIBQI`` CMake variable. The
build system will still check that the version of the sources matches the
``LIBQI_VERSION`` value if it is set.

Python
>>>>>>

The build system uses the FindPython_ CMake module. It will try to honor the
following variables if they are set:

  - ``PYTHON_VERSION_STRING``
  - ``PYTHON_LIBRARY``
  - ``PYTHON_INCLUDE_DIR``

pybind11
>>>>>>>>

The build system will by default download and compile pybind11__ as a
subproject through the ``FetchContent`` CMake module. How it is downloaded can be
customized through the following variables:

__ pybind11_repo_

  - ``PYBIND11_VERSION``
  - ``PYBIND11_GIT_REPOSITORY``
  - ``PYBIND11_GIT_TAG``


Boost
>>>>>

The build system will look for the Boost libraries on the system or in the
toolchain if one is set. The expected version of the libraries is specified as
the ``BOOST_VERSION`` variable.

The build system uses the FindBoost_ CMake module.

OpenSSL
>>>>>>>

The build system uses the FindOpenSSL_ CMake module.

ICU
>>>

The build system uses the FindICU_ CMake module.

GoogleTest
>>>>>>>>>>

The build system will by default download and compile GoogleTest__ as a
subproject through the ``FetchContent`` CMake module. How it is downloaded can be
customized through the following variables:

__ GoogleTest_repo_

  - ``GOOGLETEST_VERSION``
  - ``GOOGLETEST_GIT_REPOSITORY``
  - ``GOOGLETEST_GIT_TAG``

Install
~~~~~~~

Once the project is configured, it can be built and installed as any CMake
project:

.. code:: bash

  mkdir build && cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=/myinstallpath
  cmake --install .


Crosscompiling
--------------

The project supports cross-compiling as explained in the `CMake manual about
toolchains`__. You may simply set the ``CMAKE_TOOLCHAIN_FILE`` variable to the
path of the CMake file in your toolchain.

__ CMake_toolchains_

Testing
=======

When enabled, tests can be executed with `CTest`_.

.. code:: bash

  cd build
  ctest . --output-on-failure

.. _LibQi_repo: https://github.com/aldebaran/libqi
.. _PyPi: https://pypi.org/
.. _scikit-build: https://scikit-build.readthedocs.io/en/latest/
.. _setuptools: https://setuptools.readthedocs.io/en/latest/setuptools.html
.. _qibuild: https://github.com/aldebaran/qibuild
.. _pybind11_repo: https://pybind11.readthedocs.io/en/latest/
.. _FindPython: https://cmake.org/cmake/help/latest/module/FindPython.html
.. _FindBoost: https://cmake.org/cmake/help/latest/module/FindBoost.html
.. _FindOpenSSL: https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
.. _FindICU: https://cmake.org/cmake/help/latest/module/FindICU.html
.. _GoogleTest_repo: https://github.com/google/googletest
.. _CMake_toolchains: https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html
.. _CTest: https://cmake.org/cmake/help/latest/manual/ctest.1.html
