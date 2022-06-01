/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pysignal.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pyobject.hpp>
#include <qipython/pystrand.hpp>
#include <pybind11/pybind11.h>
#include <qi/signal.hpp>
#include <qi/anyobject.hpp>
#include <qi/periodictask.hpp>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

constexpr const auto delayArgName = "delay";

::py::object async(::py::function pyCallback,
                   ::py::args args,
                   ::py::kwargs kwargs)
{
  GILAcquire lock;

  qi::uint64_t usDelay = 0;
  if (const auto optUsDelay = extractKeywordArg<qi::uint64_t>(kwargs, delayArgName))
    usDelay = *optUsDelay;

  const MicroSeconds delay(usDelay);

  GILGuardedObject guardCb(pyCallback);
  GILGuardedObject guardArgs(std::move(args));
  GILGuardedObject guardKwargs(std::move(kwargs));

  auto invokeCallback = [=]() mutable {
    GILAcquire lock;
    auto res = ::py::object((*guardCb)(**guardArgs, ***guardKwargs)).cast<AnyValue>();
    // Release references immediately while we hold the GIL, instead of waiting
    // for the lambda destructor to relock the GIL.
    *guardKwargs = {};
    *guardArgs = {};
    *guardCb = {};
    return res;
  };

  // If there is a strand attached to that callable, we use it but we cannot use
  // asyncDelay (we use defer instead). This is because we might be executing
  // this function from inside the strand execution context, and thus asyncDelay
  // might be blocking.
  Promise prom;
  const auto strand = strandOfFunction(pyCallback);
  if (strand)
  {
    strand->defer([=]() mutable { prom.setValue(invokeCallback()); }, delay)
          .connect([=](qi::Future<void> fut) mutable {
              if (fut.hasValue()) return;
              adaptFuture(fut, prom);
            });
  }
  else
    adaptFuture(asyncDelay(invokeCallback, delay), prom);

  return castToPyObject(prom.future());
}

} // namespace

void exportAsync(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  m.def("runAsync", &async,
        "callback"_a,
        // TODO: use `::py:kwonly, ::py::arg(delayArgName) = 0` when available.
        doc(":param callback: the callback that will be called\n"
            ":param delay: an optional delay in microseconds\n"
            ":returns: a future with the return value of the function"));

  class_<PeriodicTask>(m, "PeriodicTask")
    .def(init<>())
    .def(
      "setCallback",
      [](PeriodicTask& task, ::py::function pyCallback) {
        auto callback = pyCallback.cast<std::function<void()>>();
        task.setCallback(std::move(callback));
        task.setStrand(strandOfFunction(pyCallback).get());
      },
      "callable"_a,
      doc(
        "Set the callback used by the periodic task, this function can only be "
        "called once.\n"
        ":param callable: a python callable, could be a method or a function."))
    .def("setUsPeriod",
         [](PeriodicTask& task, qi::int64_t usPeriod) {
           task.setPeriod(qi::MicroSeconds(usPeriod));
         },
         call_guard<GILRelease>(), "usPeriod"_a,
         doc("Set the call interval in microseconds.\n"
             "This call will wait until next callback invocation to apply the "
             "change.\n"
             "To apply the change immediately, use: \n"
             "\n"
             ".. code-block:: python\n"
             "\n"
             "   task.stop()\n"
             "   task.setUsPeriod(100)\n"
             "   task.start()\n"
             ":param usPeriod: the period in microseconds"))
    .def("start", &PeriodicTask::start, call_guard<GILRelease>(),
         "immediate"_a,
         doc("Start the periodic task at specified period. No effect if "
             "already running.\n"
             ":param immediate: immediate if true, first schedule of the task "
             "will happen with no delay.\n"
             ".. warning::\n"
             "   concurrent calls to start() and stop() will result in "
             "undefined behavior."))
    .def("stop", &PeriodicTask::stop, call_guard<GILRelease>(),
         doc("Stop the periodic task. When this function returns, the callback "
             "will not be called "
             "anymore. Can be called from within the callback function\n"
             ".. warning::\n"
             "   concurrent calls to start() and stop() will result in "
             "undefined behavior."))
    .def("asyncStop", &PeriodicTask::asyncStop,
         call_guard<GILRelease>(),
         doc("Request for periodic task to stop asynchronously.\n"
             "Can be called from within the callback function."))
    .def(
      "compensateCallbackTime", &PeriodicTask::compensateCallbackTime,
      call_guard<GILRelease>(), "compensate"_a,
      doc(
        ":param compensate: boolean. True to activate the compensation.\n"
        "When compensation is activated, call interval will take into account "
        "call duration to maintain the period.\n"
        ".. warning::\n"
        "   when the callback is longer than the specified period, "
        "compensation will result in the callback being called successively "
        "without pause."))
    .def("setName", &PeriodicTask::setName, call_guard<GILRelease>(),
         "name"_a, doc("Set name for debugging and tracking purpose"))
    .def("isRunning", &PeriodicTask::isRunning,
         doc(":returns: true if task is running"))
    .def("isStopping", &PeriodicTask::isStopping,
         doc("Can be called from within the callback to know if stop() or "
             "asyncStop() was called.\n"
             ":returns: whether state is stopping or stopped."));
}

} // namespace py
} // namespace qi
