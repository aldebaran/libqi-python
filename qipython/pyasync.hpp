/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYASYNC_HPP
#define QIPYTHON_PYASYNC_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportAsync(pybind11::module& module);

}
}

#endif // QIPYTHON_PYASYNC_HPP
