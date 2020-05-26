/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYTYPES_HPP
#define QIPYTHON_PYTYPES_HPP

#include <qipython/common.hpp>
#include <qi/anyvalue.hpp>

namespace qi
{
namespace py
{

pybind11::object unwrapValue(AnyReference val);

// @pre `obj != nullptr && *obj`
AnyReference unwrapAsRef(pybind11::object* obj);

void registerTypes();

}
}

#endif // QIPYTHON_PYTYPES_HPP
