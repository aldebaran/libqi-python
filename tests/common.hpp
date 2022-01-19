/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_TESTS_COMMON_HPP
#define QIPYTHON_TESTS_COMMON_HPP

#include <qipython/pyfuture.hpp>
#include <qipython/pyguard.hpp>
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
    GILAcquire lock;
    _locals.emplace();
  }

  ~Execute()
  {
    GILAcquire lock;
    _locals.reset();
  }

  void exec(const std::string& str)
  {
    GILAcquire lock;
    pybind11::exec(str, pybind11::globals(), *_locals);
  }

  pybind11::object eval(const std::string& str)
  {
    GILAcquire lock;
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

template<typename Fut>
testing::AssertionResult finishesWithValue(const Fut& fut,
                                           qi::MilliSeconds timeout = qi::MilliSeconds{ 500 })
{
  const auto state = fut.wait(timeout);
  auto result = (state == qi::FutureState_FinishedWithValue) ? testing::AssertionSuccess() :
                                                               testing::AssertionFailure();
  result << ", the future state after " << timeout.count() << "ms is " << state;

  if (state == qi::FutureState_FinishedWithError)
    result << ", the error is " << fut.error();

  return result;
}

} // namespace test
} // namespace py
} // namespace qi


#endif // QIPYTHON_TESTS_COMMON_HPP
