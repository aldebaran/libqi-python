/*
**  Copyright (C) 2023 Aldebaran Robotics
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_COMMON_HXX
#define QIPYTHON_COMMON_HXX

#include <pybind11/pybind11.h>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>

namespace qi
{
namespace py
{

template<typename F, typename... Args>
auto invokeCatchPythonError(F&& f, Args&&... args)
{
  GILAcquire acquire;
  try
  {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  }
  catch (const pybind11::error_already_set& err)
  {
    throw std::runtime_error(err.what());
  }
}

} // namespace py
} // namespace qi

#endif // QIPYTHON_COMMON_HXX
