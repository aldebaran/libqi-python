/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_QIPATH_HPP
#define QIPYTHON_QIPATH_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportPath(pybind11::module& module);

}
}

#endif // QIPYTHON_QIPATH_HPP
