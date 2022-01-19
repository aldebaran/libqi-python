/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYFUTURE_HPP
#define QIPYTHON_PYFUTURE_HPP

#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qi/future.hpp>
#include <qi/anyvalue.hpp>

namespace qi
{
namespace py
{

using Future = qi::Future<AnyValue>;
using Promise = qi::Promise<AnyValue>;

// Depending on whether we want an asynchronous result or not, returns the
// future or its value.
inline pybind11::object resultObject(const Future& fut, bool async)
{
  GILAcquire lock;
  if (async)
    return castToPyObject(fut);

  // Wait for the future outside of the GIL.
  auto res = invokeGuarded<GILRelease>(qi::SrcFuture{}, fut);
  return castToPyObject(res);
}

inline Future toFuture(qi::Future<void> f)
{
  return f.andThen(FutureCallbackType_Sync,
                   [](void*) { return AnyValue::makeVoid(); });
}

inline Future toFuture(qi::Future<AnyValue> f) { return f; }
inline Future toFuture(qi::Future<AnyReference> f)
{
  return f.andThen(FutureCallbackType_Sync,
                   [](const AnyReference& ref) { return AnyValue(ref); });
}

template<typename T>
inline Future toFuture(qi::Future<T> f)
{
  return f.andThen(FutureCallbackType_Sync,
                   [](const T& val) { return AnyValue::from(val); });
}

void exportFuture(pybind11::module& module);

} // namespace py
} // namespace qi

#endif  // QIPYTHON_PYFUTURE_HPP
