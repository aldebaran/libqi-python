/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYOBJECTFACTORY_HPP
#define QIPYTHON_PYOBJECTFACTORY_HPP

#include <qipython/common.hpp>

namespace qi
{
namespace py
{

void exportObjectFactory(pybind11::module& m);

}
}

#endif // QIPYTHON_PYOBJECTFACTORY_HPP
