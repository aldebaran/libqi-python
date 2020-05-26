/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYAPPLICATION_HPP
#define QIPYTHON_PYAPPLICATION_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportApplication(pybind11::module& m);

}
}

#endif // QIPYTHON_PYAPPLICATION_HPP
