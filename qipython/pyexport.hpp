/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYEXPORT_HPP
#define QIPYTHON_PYEXPORT_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{
  void exportAll(pybind11::module& module);
}
}

#endif  // QIPYTHON_PYEXPORT_HPP
