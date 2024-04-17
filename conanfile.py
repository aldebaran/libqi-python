from conan import ConanFile, tools
from conan.tools.cmake import cmake_layout

BOOST_COMPONENTS = [
    "atomic",
    "chrono",
    "container",
    "context",
    "contract",
    "coroutine",
    "date_time",
    "exception",
    "fiber",
    "filesystem",
    "graph",
    "graph_parallel",
    "iostreams",
    "json",
    "locale",
    "log",
    "math",
    "mpi",
    "nowide",
    "program_options",
    "python",
    "random",
    "regex",
    "serialization",
    "stacktrace",
    "system",
    "test",
    "thread",
    "timer",
    "type_erasure",
    "wave",
]

USED_BOOST_COMPONENTS = [
    "thread",
    # required by libqi
    "filesystem",
    "locale",
    "program_options",
    "random",
    "regex",
    # required by boost.thread
    "atomic",
    "chrono",
    "container",
    "date_time",
    "exception",
    "system",
]


class QiPythonConan(ConanFile):
    requires = [
        "boost/[~1.83]",
        "pybind11/[^2.11]",
        "qi/[~4]",
    ]

    test_requires = [
        "gtest/[~1.14]",
    ]

    generators = "CMakeToolchain", "CMakeDeps"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    default_options = {
        "boost/*:shared": True,
        "openssl/*:shared": True,
        "qi/*:with_boost_locale": True,  # for `pytranslator.cpp`
    }

    # Disable every components of Boost unless we actively use them.
    default_options.update(
        {
            f"boost/*:without_{_name}": (
                False if _name in USED_BOOST_COMPONENTS else True
            )
            for _name in BOOST_COMPONENTS
        }
    )

    def layout(self):
        # Configure the format of the build folder name, based on the value of some variables.
        self.folders.build_folder_vars = [
            "settings.os",
            "settings.arch",
            "settings.compiler",
            "settings.build_type",
        ]

        # The cmake_layout() sets the folders and cpp attributes to follow the
        # structure of a typical CMake project.
        cmake_layout(self)
