/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyfuture.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pystrand.hpp>
#include <qi/future.hpp>
#include <qi/anyobject.hpp>
#include <pybind11/pybind11.h>

static constexpr const auto logCategory = "qi.python.future";
qiLogCategory(logCategory);

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

Future futureBarrier(std::vector<Future> futs)
{
  auto waitFut = waitForAll(futs).async();
  Promise prom(boost::bind(waitFut.makeCanceler()));
  waitFut.andThen([=](const std::vector<Future>& futs) mutable {
    prom.setValue(AnyValue::from(futs));
  });
  return prom.future();
}

// A function to cast a Python object into a C++ object, unless R is void, in
// which case does nothing.
//
// pybind11 will fail to cast an object to void if the object is not `None` or
// a capsule, but all we want is to ignore the result if the return value is
// expected to be void.
template<typename R>
R castIfNotVoid(const ::py::object& obj)
{
  return ::py::cast<R>(obj);
}

template<>
void castIfNotVoid<void>(const ::py::object&) {}

template<typename R, typename... Args>
std::function<qi::Future<R>(Args...)> toContinuation(const ::py::function& cb)
{
  GILAcquire lock;
  GILGuardedObject guardedCb(cb);
  auto callGuardedCb = [=](Args... args) mutable {
    GILAcquire lock;
    const auto handleExcept = ka::handle_exception_rethrow(
      exceptionLogVerbose(
        logCategory,
        "An exception occurred while executing a future continuation"),
      ka::type_t<::py::object>{});
    const auto pyRes =
      ka::invoke_catch(handleExcept, *guardedCb, std::forward<Args>(args)...);
    // Release references immediately while we hold the GIL, instead of waiting
    // for the lambda destructor to relock the GIL.
    *guardedCb = {};
    return castIfNotVoid<R>(pyRes);
  };

  auto strand = strandOfFunction(cb);
  if (strand)
    return strand->schedulerFor(std::move(callGuardedCb));
  return futurizeOutput(std::move(callGuardedCb));
}

void addCallback(Future fut, const ::py::function& cb)
{
  auto cont = toContinuation<void, Future>(cb);
  GILRelease _unlock;
  fut.connect(std::move(cont));
}

Future then(Future fut, const ::py::function& cb)
{
  auto cont = toContinuation<AnyValue, Future>(cb);
  GILRelease _unlock;
  return fut.then(std::move(cont)).unwrap();
}

Future andThen(Future fut, const ::py::function& cb)
{
  auto cont = toContinuation<AnyValue, AnyValue>(cb);
  GILRelease _unlock;
  return fut.andThen(std::move(cont)).unwrap();
}

Future unwrap(Future fut)
{
  Promise prom(boost::bind(fut.makeCanceler()));
  fut.connect([=](qi::Future<AnyValue> fut) mutable {
    if (!fut.hasValue())
    {
      adaptFuture(fut, prom);
      return;
    }

    AnyReference ref = fut.value().asReference();
    while (ref.kind() == TypeKind_Dynamic)
      ref = ref.content();

    AnyValue val(ref);
    if (!qi::detail::handleFuture(val.asReference(), prom))
    {
      std::ostringstream ss;
      ss << "Unwrapping something that is not a nested future: "
        << ref.type()->infoString();
      qiLogWarning() << ss.str();
      prom.setError(ss.str());
      return;
    }

    // `handleFuture` takes ownership of the value on success, we can release it.
    val.release();
  });
  return prom.future();
}

} // namespace

void exportFuture(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  enum_<FutureState>(m, "FutureState")
      .value("None", FutureState_None)
      .value("Running", FutureState_Running)
      .value("Canceled", FutureState_Canceled)
      .value("FinishedWithError", FutureState_FinishedWithError)
      .value("FinishedWithValue", FutureState_FinishedWithValue);

  enum_<FutureTimeout>(m, "FutureTimeout")
      .value("None", FutureTimeout_None)
      .value("Infinite", FutureTimeout_Infinite);

  class_<Promise>(m, "Promise")
      .def(
        init([](std::function<void(Promise&)> onCancel) {
          Promise prom;
          if (onCancel)
          {
            prom.setOnCancel([=](Promise& prom) {
              ka::invoke_catch(
                exceptionLogWarning("qipy.future",
                                    "Promise `onCancel` callback threw an exception"),
                [&] { onCancel(prom); });
            });
          }
          return prom;
        }),
        call_guard<GILRelease>(),
        "on_cancel"_a = none(),
        doc(":param on_cancel: a function that will be called when a cancel is requested on the future.\n"))

      .def("setCanceled", &Promise::setCanceled,
           call_guard<GILRelease>(),
           doc("Set the state of the promise to Canceled."))

      .def("setError", &Promise::setError,
           call_guard<GILRelease>(),
           "error"_a,
           doc("Set the error of the promise."))

      .def("setValue", &Promise::setValue,
           call_guard<GILRelease>(),
           "value"_a,
           doc("Set the value of the promise."))

      .def("future", &Promise::future,
           call_guard<GILRelease>(),
           doc("Get a future tied to the promise. You can get multiple futures from the same promise."))

      .def("isCancelRequested", &Promise::isCancelRequested,
           call_guard<GILRelease>(),
           doc(":returns: True if the future associated with the promise asked for cancellation."));

  class_<Future>(m, "Future")
      .def(init<AnyValue>(),
           doc("Create a future with a value."))

      .def("value",
           static_cast<const AnyValue&(Future::*)(int) const>(&Future::value),
           call_guard<GILRelease>(),
           "timeout"_a = FutureTimeout_Infinite,
           doc("Block until the future is ready.\n\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":returns: the value of the future.\n"
               ":raises: a RuntimeError if the timeout is reached or the future has error."))

      .def("error", &Future::error,
           call_guard<GILRelease>(),
           "timeout"_a = FutureTimeout_Infinite,
           doc("Block until the future is ready.\n\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":returns: the error of the future.\n"
               ":raises: a RuntimeError if the timeout is reached or the future has no error."))


      .def("wait",
           static_cast<FutureState(Future::*)(int) const>(&Future::wait),
           call_guard<GILRelease>(),
           "timeout"_a = FutureTimeout_Infinite,
           doc("Wait for the future to be ready.\n\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":returns: a :data:`qi.FutureState`."))

      .def("hasError", &Future::hasError,
           call_guard<GILRelease>(),
           "timeout"_a = FutureTimeout_Infinite,
           doc(":param timeout: a time in milliseconds. Optional.\n"
               ":returns: true iff the future has an error.\n"
               ":raise: a RuntimeError if the timeout is reached."))

      .def("hasValue", &Future::hasValue,
           call_guard<GILRelease>(),
           "timeout"_a = FutureTimeout_Infinite,
           doc(":param timeout: a time in milliseconds. Optional.\n"
               ":returns: true iff the future has a value.\n"
               ":raise: a RuntimeError if the timeout is reached."))

      .def("cancel", &Future::cancel,
           call_guard<GILRelease>(),
           doc("Ask for cancellation."))

      .def("isFinished", &Future::isFinished,
           call_guard<GILRelease>(),
           doc("Return true if the future is not running anymore (i.e. if hasError or hasValue or isCanceled)."))

      .def("isRunning", &Future::isRunning,
           call_guard<GILRelease>(),
           doc("Return true if the future is still running."))

      .def("isCanceled", &Future::isCanceled,
           call_guard<GILRelease>(),
           doc("Return true if the future is canceled."))

      .def("isCancelable", [](const Future&) { return true; },
           call_guard<GILRelease>(),
           doc(":returns: always true, all future are cancellable now\n"
               ".. deprecated:: 1.5.0\n"))

      .def("addCallback", &qi::py::addCallback,
           "callback"_a,
           doc("Add a callback that will be called when the future becomes ready.\n\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the future itself.\n\n"
               ":param callback: a python callable, could be a method or a function."))

      .def("then", &qi::py::then,
           "callback"_a,
           doc("Add a callback that will be called when the future becomes ready.\n\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the future itself.\n\n"
               ":param callback: a python callable, could be a method or a function.\n"
               ":returns: a future that will contain the return value of the callback."))

      .def("andThen", &qi::py::andThen,
           "callback"_a,
           doc("Add a callback that will be called when the future becomes ready if it has a value.\n\n"
               "If the future finishes with an error, the callback is not called and the future returned by "
               "andThen is set to that error.\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the value of the future itself.\n\n"
               ":param callback: a python callable, could be a method or a function.\n"
               ":returns: a future that will contain the return value of the callback."))

      .def("unwrap", &qi::py::unwrap,
           call_guard<GILRelease>(),
           doc("If this is a Future of a Future of X, return a Future of X.\n\n"
               "The state of both futures is forwarded and cancel requests are forwarded to the appropriate future."));

  m.def("futureBarrier", &qi::py::futureBarrier,
        call_guard<GILRelease>(),
        doc("Return a future that will be set with all the futures given as argument when they are\n"
            " all finished. This is useful to wait for a bunch of Futures at once.\n\n"
            ":param futureList: A list of Futures to wait for\n"
            ":returns: A Future of list of futureList."));
}

} // namespace py
} // namespace qi
