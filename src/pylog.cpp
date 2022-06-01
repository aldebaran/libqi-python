/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pylog.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qi/application.hpp>
#include <qi/log.hpp>
#include <qi/os.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace qi
{
namespace py
{

void exportLog(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  enum_<LogLevel>(m, "LogLevel")
    .value("Silent",  LogLevel_Silent)
    .value("Fatal",   LogLevel_Fatal)
    .value("Error",   LogLevel_Error)
    .value("Warning", LogLevel_Warning)
    .value("Info",    LogLevel_Info)
    .value("Verbose", LogLevel_Verbose)
    .value("Debug",   LogLevel_Debug);

  m.def("pylog",
        [](LogLevel level, const std::string& name, const std::string& message,
           const std::string& file, const std::string& func, int line) {
          log::log(level, name.c_str(), message.c_str(), file.c_str(),
                   func.c_str(), line);
        },
        call_guard<GILRelease>(), "level"_a, "name"_a, "message"_a,
        "file"_a, "func"_a, "line"_a);

  m.def("setFilters",
        [](const std::string& filters) { log::addFilters(filters); },
        call_guard<GILRelease>(), "filters"_a,
        doc("Set log filtering options.\n"
            "Each rule can be:\n\n"
            "  +CAT: enable category CAT\n\n"
            "  -CAT: disable category CAT\n\n"
            "  CAT=level : set category CAT to level\n\n"
            "Each category can include a '*' for globbing.\n"
            "\n"
            ".. code-block:: python\n"
            "\n"
            "  qi.logging.setFilter(\"qi.*=debug:-qi.foo:+qi.foo.bar\")\n"
            "\n"
            "(all qi.* logs in info, remove all qi.foo logs except qi.foo.bar)\n"
            ":param filters: List of rules separated by colon."));

  m.def("setContext", [](int context) { qi::log::setContext(context); },
        call_guard<GILRelease>(), "context"_a,
        doc("  1  : Verbosity                            \n"
            "  2  : ShortVerbosity                       \n"
            "  4  : Date                                 \n"
            "  8  : ThreadId                             \n"
            "  16 : Category                             \n"
            "  32 : File                                 \n"
            "  64 : Function                             \n"
            "  128: EndOfLine                            \n\n"
            "  Some useful values for context are:       \n"
            "  26 : (verb+threadId+cat)                  \n"
            "  30 : (verb+threadId+date+cat)             \n"
            "  126: (verb+threadId+date+cat+file+fun)    \n"
            "  254: (verb+threadId+date+cat+file+fun+eol)\n\n"
            ":param context: A bitfield (sum of described values)."));

  m.def("setLevel", [](LogLevel level) { log::setLogLevel(level); },
        call_guard<GILRelease>(), "level"_a,
        doc(
          "Sets the threshold for the logger to level. "
          "Logging messages which are less severe than level will be ignored. "
          "Note that the logger is created with level INFO.\n"
          ":param level: The minimum log level."));
}
} // namespace py
} // namespace qi
