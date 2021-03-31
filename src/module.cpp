/*
**  Copyright (C) 2020 Softbank Robotics Europe
**  See COPYING for the license
*/

#include <pybind11/pybind11.h>
#include <qi/log.hpp>
#include <qipython/common.hpp>
#include <qipython/pyexport.hpp>

namespace py = pybind11;

PYBIND11_MODULE(qi_python, module)
{
  py::options options;
  options.enable_user_defined_docstrings();

  module.doc() = "LibQi bindings for Python.";
  qi::py::exportAll(module);
}
