/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pysignal.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pystrand.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pyobject.hpp>
#include <qi/signal.hpp>
#include <qi/anyobject.hpp>

qiLogCategory("qi.python.signal");

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

constexpr static const auto asyncArgName = "_async";

AnyReference dynamicCallFunction(const GILGuardedObject& func,
                                 const AnyReferenceVector& args)
{
  GILAcquire lock;
  ::py::list pyArgs(args.size());
  ::py::size_t i = 0;
  for (const auto& arg : args)
    pyArgs[i++] = castToPyObject(arg);
  (*func)(*pyArgs);
  return AnyValue::makeVoid().release();
}

// Procedure<Future<SignalLink>(SignalSubscriber)> F
template<typename F>
::py::object connect(F&& connect,
                     const ::py::function& pyCallback,
                     bool async)
{
  GILAcquire lock;
  const auto strand = strandOfFunction(pyCallback);

  SignalSubscriber subscriber(AnyFunction::fromDynamicFunction(
                                boost::bind(dynamicCallFunction,
                                            GILGuardedObject(pyCallback),
                                            _1)),
                              strand.get());
  subscriber.setCallType(MetaCallType_Auto);
  const auto fut = std::forward<F>(connect)(std::move(subscriber))
                     .andThen(FutureCallbackType_Sync, [](SignalLink link) {
                       return AnyValue::from(link);
                     });
  return resultObject(fut, async);
}

::py::object proxySignalConnect(detail::ProxySignal& sig,
                                const ::py::function& callback,
                                bool async)
{
  GILAcquire lock;
  return detail::proxySignalConnect(sig.object, sig.signalId, callback, async);
}

} // namespace

namespace detail
{

detail::ProxySignal::~ProxySignal()
{
  // The destructor can lock waiting for callbacks to end.
  GILRelease unlock;
  object.reset();
}


::py::object signalConnect(SignalBase& sig,
                           const ::py::function& callback,
                           bool async)
{
  GILAcquire lock;
  return connect(
    [&](const SignalSubscriber& sub) {
      GILRelease unlock;
      return sig.connectAsync(sub).andThen(FutureCallbackType_Sync,
                                           [](const SignalSubscriber& sub) {
                                             return sub.link();
                                           });
    },
    callback, async);
}

::py::object signalDisconnect(SignalBase& sig, SignalLink id, bool async)
{
  const auto fut =
      sig.disconnectAsync(id).andThen(FutureCallbackType_Sync, [](bool success) {
    return AnyValue::from(success);
  });

  GILAcquire lock;
  return resultObject(fut, async);
}

::py::object signalDisconnectAll(SignalBase& sig, bool async)
{
  const auto fut =
      sig.disconnectAllAsync().andThen(FutureCallbackType_Sync, [](bool success) {
    return AnyValue::from(success);
  });

  GILAcquire lock;
  return resultObject(fut, async);
}

::py::object proxySignalConnect(const AnyObject& obj,
                                unsigned int signalId,
                                const ::py::function& callback,
                                bool async)
{
  GILAcquire lock;
  return connect(
    [&](const SignalSubscriber& sub) {
      GILRelease unlock;
      return obj.connect(signalId, sub).async();
    },
    callback, async);
}

::py::object proxySignalDisconnect(const AnyObject& obj,
                                   SignalLink id,
                                   bool async)
{
  const auto fut = [&] {
    GILRelease unlock;
    return toFuture(obj.disconnect(id));
  }();
  GILAcquire lock;
  return resultObject(fut, async);
}

} // namespace detail

bool isSignal(const ::py::object& obj)
{
  GILAcquire lock;
  return ::py::isinstance<Signal>(obj) ||
         ::py::isinstance<detail::ProxySignal>(obj);
}

void exportSignal(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  using SignalPtr = std::unique_ptr<Signal, DeleteOutsideGIL>;
  class_<Signal, SignalPtr>(m, "Signal")

    .def(init([](std::string signature, std::function<void(bool)> onConnect) {
           qi::SignalBase::OnSubscribers onSub;
           if (onConnect)
             onSub = qi::futurizeOutput(onConnect);
           return new Signal(signature, onSub);
         }),
         call_guard<GILRelease>(),
         "signature"_a = "m", "onConnect"_a = std::function<void(bool)>())

    .def(
      "connect", &detail::signalConnect,
      call_guard<GILRelease>(),
      "callback"_a, arg(asyncArgName) = false,
      doc(
        "Connect the signal to a callback, the callback will be called each "
        "time the signal is triggered. Use the id returned to unregister the "
        "callback."
        ":param callback: the callback that will be called when the signal is "
        "triggered.\n"
        ":returns: the connection id of the registered callback."))

    .def("disconnect", &detail::signalDisconnect,
         call_guard<GILRelease>(), "id"_a, arg(asyncArgName) = false,
         doc("Disconnect the callback associated to id.\n"
             ":param id: the connection id returned by connect.\n"
             ":returns: true on success."))

    .def("disconnectAll", &detail::signalDisconnectAll,
         call_guard<GILRelease>(), arg(asyncArgName) = false,
         doc("Disconnect all subscribers associated to the property.\n\n"
             "This function should be used with caution, as it may also remove "
             "subscribers that were added by other callers.\n\n"
             ":returns: true on success\n"))

    .def("__call__",
         [](Signal& sig, args pyArgs) {
           const auto args =
             AnyReference::from(pyArgs).content().asTupleValuePtr();
           GILRelease unlock;
           sig.trigger(args);
         },
         doc("Trigger the signal"));

  class_<detail::ProxySignal>(m, "_ProxySignal")
    .def("connect", &proxySignalConnect, "callback"_a,
         arg(asyncArgName) = false)
    .def("disconnect",
         [](detail::ProxySignal& sig, SignalLink id, bool async) {
           return detail::proxySignalDisconnect(sig.object, id, async);
         },
         "id"_a, arg(asyncArgName) = false)
    .def("__call__",
         [](detail::ProxySignal& sig, args pyArgs) {
           const auto args =
             AnyReference::from(pyArgs).content().asTupleValuePtr();
           GILRelease unlock;
           sig.object.metaPost(sig.signalId, args);
  });
}

} // namespace py
} // namespace qi
