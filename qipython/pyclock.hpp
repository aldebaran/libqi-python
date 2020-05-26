/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYCLOCK_HPP
#define QIPYTHON_PYCLOCK_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportClock(pybind11::module& module);

} // namespace py
} // namespace qi

#endif // QIPYTHON_PYCLOCK_HPP
