/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYTRANSLATOR_HPP
#define QIPYTHON_PYTRANSLATOR_HPP

#include <qipython/common.hpp>
#include <qi/translator.hpp>

namespace qi
{
namespace py
{

void exportTranslator(pybind11::module& module);

} // namespace py
} // namespace qi

#endif // QIPYTHON_PYTRANSLATOR_HPP
