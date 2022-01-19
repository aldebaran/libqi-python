/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/


#include <qipython/pypath.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <qi/path.hpp>
#include <qi/anyvalue.hpp>

namespace py = pybind11;

namespace qi
{
namespace py
{

void exportPath(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  m.def("sdkPrefix", &path::sdkPrefix, call_guard<GILRelease>(),
        doc(":returns: The SDK prefix path. It is always a complete, native "
            "path.\n"));

  m.def(
    "findBin", &path::findBin, call_guard<GILRelease>(), "name"_a,
    "searchInPath"_a = false,
    doc("Look for a binary in the system.\n"
        ":param name: string. The full name of the binary, or just the name.\n"
        ":param searchInPath: boolean. Search in $PATH if it hasn't been "
        "found in sdk dirs. Optional.\n"
        ":returns: the complete, native path to the file found. An empty string "
        "otherwise."));

  m.def(
    "findLib", &path::findLib, call_guard<GILRelease>(), "name"_a,
    doc("Look for a library in the system.\n"
        ":param name: string. The full name of the library, or just the name.\n"
        ":returns: the complete, native path to the file found. An empty string "
        "otherwise."));

  m.def(
    "findConf", &path::findConf, call_guard<GILRelease>(),
    "application"_a, "file"_a, "excludeUserWritablePath"_a = false,
    doc("Look for a configuration file in the system.\n"
        ":param application: string. The name of the application.\n"
        ":param file: string. The name of the file to look for."
        " You can specify subdirectories using '/' as a separator.\n"
        ":param excludeUserWritablePath: If true, findConf() won't search into "
        "userWritableConfPath.\n"
        ":returns: the complete, native path to the file found. An empty string "
        "otherwise."));

  m.def(
    "findData", &path::findData, call_guard<GILRelease>(),
    "application"_a, "file"_a, "excludeUserWritablePath"_a = false,
    doc("Look for a file in all dataPaths(application) directories. Return the "
        "first match.\n"
        ":param application: string. The name of the application.\n"
        ":param file: string. The name of the file to look for."
        " You can specify subdirectories using a '/' as a separator.\n"
        ":param excludeUserWritablePath: If true, findData() won't search into "
        "userWritableDataPath.\n"
        ":returns: the complete, native path to the file found. An empty string "
        "otherwise."));

  m.def(
    "listData",
    [](const std::string& applicationName, const std::string& pattern) {
      return path::listData(applicationName, pattern);
    },
    call_guard<GILRelease>(), "applicationName"_a, "pattern"_a,
    doc("List data files matching the given pattern in all "
        "dataPaths(application) directories.\n"
        " For each match, return the occurrence from the first dataPaths prefix."
        " Directories are discarded.\n\n"
        ":param application: string. The name of the application.\n"
        ":param patten: string. Wildcard pattern of the files to look for."
        " You can specify subdirectories using a '/' as a separator."
        " \"*\" by default.\n"
        ":returns: a list of the complete, native paths of the files that "
        "matched."));

  m.def("listData", [](const std::string& applicationName) {
          return path::listData(applicationName);
        },
        call_guard<GILRelease>(), "applicationName"_a);

  m.def("confPaths", [](const std::string& applicationName) {
          return path::confPaths(applicationName);
        },
        call_guard<GILRelease>(),
        "applicationName"_a,
        doc("Get the list of directories used when searching for "
            "configuration files for the given application.\n"
            ":param applicationName: string. Name of the application. "
            "\"\" by default.\n"
            ":returns: The list of configuration directories.\n"
            ".. warning::\n"
            "   You should not assume those directories exist,"
            " nor that they are writable."));

  m.def("confPaths", [] { return path::confPaths(); },
        call_guard<GILRelease>());

  m.def("dataPaths", [](const std::string& applicationName) {
          return path::dataPaths(applicationName);
        },
        call_guard<GILRelease>(),
        "applicationName"_a,
        doc("Get the list of directories used when searching for "
            "configuration files for the given application.\n"
            ":param application: string. Name of the application. "
            "\"\" by default.\n"
            ":returns: The list of data directories.\n"
            ".. warning::\n"
            "   You should not assume those directories exist,"
            " nor that they are writable."));

  m.def("dataPaths", [] { return path::dataPaths(); },
        call_guard<GILRelease>());

  m.def("binPaths", [] { return path::binPaths(); },
        call_guard<GILRelease>(),
        doc(
          ":returns: The list of directories used when searching for binaries.\n"
          ".. warning::\n"
          "   You should not assume those directories exist,"
          " nor that they are writable."));

  m.def(
    "libPaths", [] { return path::libPaths(); },
    call_guard<GILRelease>(),
    doc(":returns: The list of directories used when searching for libraries.\n\n"
        ".. warning::\n"
        "   You should not assume those directories exist,"
        " nor that they are writable."));

  m.def("setWritablePath", &path::detail::setWritablePath,
        call_guard<GILRelease>(), "path"_a,
        doc("Set the writable files path for users.\n"
            ":param path: string. A path on the system.\n"
            "Use an empty path to reset it to its default value."));

  m.def("userWritableDataPath", &path::userWritableDataPath,
        call_guard<GILRelease>(), "applicationName"_a, "fileName"_a,
        doc("Get the writable data files path for users.\n"
            ":param applicationName: string. Name of the application.\n"
            ":param fileName: string. Name of the file.\n"
            ":returns: The file path."));

  m.def("userWritableConfPath", &path::userWritableConfPath,
        call_guard<GILRelease>(), "applicationName"_a, "fileName"_a,
        doc("Get the writable configuration files path for users.\n"
            ":param applicationName: string. Name of the application.\n"
            ":param fileName: string. Name of the file.\n"
            ":returns: The file path."));

  m.def("sdkPrefixes", [] { return path::detail::getSdkPrefixes(); },
        call_guard<GILRelease>(),
        doc("List of SDK prefixes.\n"
            ":returns: The list of sdk prefixes."));

  m.def("addOptionalSdkPrefix", &path::detail::addOptionalSdkPrefix,
        call_guard<GILRelease>(), "prefix"_a,
        doc("Add a new SDK path.\n"
            ":param: an sdk prefix."));

  m.def("clearOptionalSdkPrefix", &path::detail::clearOptionalSdkPrefix,
        call_guard<GILRelease>(),
        doc("Clear all optional sdk prefixes."));
}

} // namespace py
} // namespace qi
