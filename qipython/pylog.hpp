/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYLOG_HPP
#define QIPYTHON_PYLOG_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportLog(pybind11::module& module);

}
} // namespace qi

#endif // QIPYTHON_PYLOG_HPP
