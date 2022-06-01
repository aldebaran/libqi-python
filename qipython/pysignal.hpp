/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYSIGNAL_HPP
#define QIPYTHON_PYSIGNAL_HPP

#include <qipython/common.hpp>
#include <qi/type/typeinterface.hpp>
#include <qi/type/metasignal.hpp>
#include <qi/anyobject.hpp>
#include <memory>

namespace qi
{
namespace py
{

using Signal = qi::SignalBase;
using SignalPtr = std::shared_ptr<Signal>;

namespace detail
{

/// The destructor of `AnyObject` can block waiting for callbacks to end.
/// This type ensures that it's always called while the GIL is unlocked.
struct AnyObjectGILSafe
{
  AnyObjectGILSafe() = default;

  /// @throws `std::runtime_error` if the weak object is expired.
  /// @post With s = AnyObjectGILSafe(weak), s->isValid()
  explicit AnyObjectGILSafe(AnyWeakObject weak);

  // Copy/Move operations are safe to default because they will never cause the
  // destruction of the `AnyObject` contained in either the source or the
  // destination (this) object.
  // The declaration are still kept to enforce the 3/5/0 rule.
  AnyObjectGILSafe(const AnyObjectGILSafe& o) = default;
  AnyObjectGILSafe& operator=(const AnyObjectGILSafe& o) = default;
  AnyObjectGILSafe(AnyObjectGILSafe&& o) = default;
  AnyObjectGILSafe& operator=(AnyObjectGILSafe&& o) = default;

  ~AnyObjectGILSafe();

  inline AnyObject& operator*() { return _obj; }
  inline const AnyObject& operator*() const { return _obj; }

  inline AnyObject* operator->() { return &_obj; }
  inline const AnyObject* operator->() const { return &_obj; }

private:
  AnyObject _obj;
};

struct ProxySignal
{
  AnyWeakObject object;
  unsigned int signalId;
};

pybind11::object signalConnect(SignalBase& sig,
                               const pybind11::function& pyCallback,
                               bool async);

pybind11::object signalDisconnect(SignalBase& sig, SignalLink id, bool async);

pybind11::object signalDisconnectAll(SignalBase& sig, bool async);

pybind11::object proxySignalConnect(const AnyObjectGILSafe& obj,
                                    unsigned int signalId,
                                    const pybind11::function& callback,
                                    bool async);

pybind11::object proxySignalDisconnect(const AnyObjectGILSafe& obj,
                                       SignalLink id,
                                       bool async);
} // namespace detail

// Checks if an object is an instance of a signal.
bool isSignal(const pybind11::object& obj);

void exportSignal(pybind11::module& m);

} // namespace py
} // namespace qi

#endif // QIPYTHON_PYSIGNAL_HPP
