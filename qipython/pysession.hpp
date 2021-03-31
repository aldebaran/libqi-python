/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYSESSION_HPP
#define QIPYTHON_PYSESSION_HPP

#include <qipython/common.hpp>
#include <qi/session.hpp>

namespace qi
{
namespace py
{

pybind11::object makeSession(SessionPtr sess);

void exportSession(pybind11::module& module);

} // namespace py
} // namespace qi

#endif  // QIPYTHON_PYSESSION_HPP
