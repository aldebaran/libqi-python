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

struct ProxySignal
{
  AnyObject object;
  unsigned int signalId;

  ~ProxySignal();
};

pybind11::object signalConnect(SignalBase& sig,
                               const pybind11::function& pyCallback,
                               bool async);

pybind11::object signalDisconnect(SignalBase& sig, SignalLink id, bool async);

pybind11::object signalDisconnectAll(SignalBase& sig, bool async);

pybind11::object proxySignalConnect(const AnyObject& obj,
                                    unsigned int signalId,
                                    const pybind11::function& callback,
                                    bool async);

pybind11::object proxySignalDisconnect(const AnyObject& obj,
                                       SignalLink id,
                                       bool async);
} // namespace detail

// Checks if an object is an instance of a signal.
bool isSignal(const pybind11::object& obj);

void exportSignal(pybind11::module& m);

} // namespace py
} // namespace qi

#endif // QIPYTHON_PYSIGNAL_HPP
