/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_TESTS_COMMON_HPP
#define QIPYTHON_TESTS_COMMON_HPP

#include <qipython/pyfuture.hpp>
#include <gtest/gtest.h>
#include <boost/optional.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

namespace qi
{
namespace py
{
namespace test
{

struct CheckPoint
{
  CheckPoint(int count = 1) : count(count) {}
  CheckPoint(qi::Promise<void> p) : count(1), prom(p) {}

  CheckPoint(const CheckPoint&) = delete;
  CheckPoint& operator=(const CheckPoint&) = delete;

  std::atomic_int count;
  qi::Promise<void> prom = qi::Promise<void>();
  template<typename... Args>
  void operator()(Args&&...)
  {
    if (--count == 0)
      prom.setValue(nullptr);
  }
  qi::Future<void> fut() const { return prom.future(); }
};

struct Execute
{
  Execute()
  {
    pybind11::gil_scoped_acquire lock;
    _locals.emplace();
  }

  ~Execute()
  {
    pybind11::gil_scoped_acquire lock;
    _locals.reset();
  }

  void exec(const std::string& str)
  {
    pybind11::gil_scoped_acquire lock;
    pybind11::exec(str, pybind11::globals(), *_locals);
  }

  pybind11::object eval(const std::string& str)
  {
    pybind11::gil_scoped_acquire lock;
    return pybind11::eval(str, pybind11::globals(), *_locals);
  }

  const pybind11::dict& locals() const
  {
    return *_locals;
  }

private:
  boost::optional<pybind11::dict> _locals;
};

template<typename T>
qi::Future<T> toFutOf(qi::py::Future fut)
{
  return fut.andThen([](const qi::AnyValue& val){ return val.to<T>(); });
}

}
}
}


#endif // QIPYTHON_TESTS_COMMON_HPP
