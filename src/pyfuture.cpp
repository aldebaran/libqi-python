/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyfuture.hpp>
#include <qi/future.hpp>
#include <qi/anyobject.hpp>
#include <qipython/gil.hpp>
#include <boost/python.hpp>
#include <boost/foreach.hpp>
#include <boost/function_types/components.hpp>
#include <boost/function_types/function_pointer.hpp>
#include <qipython/error.hpp>
#include <qipython/pythreadsafeobject.hpp>
#include "pystrand.hpp"

qiLogCategory("qipy.future");

namespace qi {
  namespace py {

    void onBarrierFinished(const std::vector<qi::Future<qi::AnyValue> >& futs, Promise<AnyValue> prom)
    {
      GILScopedLock _lock;
      boost::python::list list;
      for (const auto& f : futs)
        list.append(boost::python::object(PyFuture(f)));
      prom.setValue(AnyValue::from(list));
    }

    boost::python::object pyFutureBarrier(boost::python::list l)
    {
      std::vector<qi::Future<qi::AnyValue> > futs;
      for (long i = 0; i < boost::python::len(l); ++i)
      {
        boost::python::extract<PyFuture*> ex(l[i]);
        if (!ex.check())
          throw std::runtime_error("Not a future");

        futs.push_back(*ex());
      }
      auto fut = waitForAll(futs).async();
      Promise<AnyValue> prom;
      prom.setOnCancel(boost::bind(fut.makeCanceler()));
      fut.andThen(boost::bind(&onBarrierFinished, _1, prom));
      return boost::python::object(PyFuture(prom.future()));
    }

    void pyFutureCb(const qi::Future<qi::AnyValue>& fut, const PyThreadSafeObject& callable) {
      GILScopedLock _lock;
      //reconstruct a pyfuture from the c++ future (the c++ one is always valid here)
      //both pypromise and pyfuture could have disappeared here.
      PY_CATCH_ERROR(callable.object()(PyFuture(fut)));
    }

    qi::AnyValue pyFutureThen(const qi::Future<qi::AnyValue>& fut, const PyThreadSafeObject& callable) {
      GILScopedLock _lock;
      boost::python::object ret;
      try {
        ret = callable.object()(PyFuture(fut));
      }
      catch (boost::python::error_already_set& e) {
        std::string s = PyFormatError();
        throw std::runtime_error(s);
      }
      return qi::AnyValue::from(ret);
    }

    qi::AnyValue pyFutureAndThen(const qi::AnyValue& val, const PyThreadSafeObject& callable) {
      GILScopedLock _lock;
      boost::python::object ret;
      try {
        ret = callable.object()(val.to<boost::python::object>());
      }
      catch (boost::python::error_already_set& e) {
        std::string s = PyFormatError();
        throw std::runtime_error(s);
      }
      return qi::AnyValue::from(ret);
    }

    static void pyFutureUnwrap(const qi::Future<qi::AnyValue>& fut,
        qi::Promise<AnyValue> promise)
    {
      if (fut.isCanceled())
        promise.setCanceled();
      else if (fut.hasError())
        promise.setError(fut.error());

      AnyReference ref = fut.value().asReference();
      while (ref.kind() == qi::TypeKind_Dynamic)
        ref = ref.content();
      // handleFuture() below does a destroy() on the ref. remove this clone() when we have c++11 and we use anyvalue
      // everywhere
      ref = ref.clone();
      if (!qi::detail::handleFuture(ref, promise))
      {
        std::ostringstream ss;
        ss << "Unwrapping something that is not a nested future: "
          << ref.type()->infoString();
        ref.destroy();
        qiLogWarning() << ss.str();
        promise.setError(ss.str());
      }
    }

    PyFuture::PyFuture()
    {}

    PyFuture::PyFuture(const boost::python::object& obj)
      : qi::Future<qi::AnyValue>(qi::AnyValue::from(obj))
    {}

    PyFuture::PyFuture(const qi::Future<qi::AnyValue>& fut)
      : qi::Future<qi::AnyValue>(fut)
    {}

    boost::python::object PyFuture::value(int msecs) const {
      qi::AnyValue gv;
      {
        GILScopedUnlock _unlock;
        //throw in case of error
        gv = qi::Future<qi::AnyValue>::value(msecs);
      }
      return gv.to<boost::python::object>();
    }

    void PyFuture::addCallback(const boost::python::object &callable) {
      if (!PyCallable_Check(callable.ptr()))
        throw std::runtime_error("Not a callable");

      PyThreadSafeObject obj(callable);

      qi::Strand* strand = extractStrandFromCallable(callable);
      if (strand)
      {
        GILScopedUnlock _unlock;
        connect(strand->schedulerFor(boost::bind(&pyFutureCb, _1, obj)));
      }
      else
      {
        GILScopedUnlock _unlock;
        connect(boost::bind(&pyFutureCb, _1, obj));
      }
    }

    boost::python::object PyFuture::pyThen(
        const boost::python::object& callable)
    {
      if (!PyCallable_Check(callable.ptr()))
        throw std::runtime_error("Not a callable");

      PyThreadSafeObject obj(callable);

      qi::Future<AnyValue> fut;
      qi::Strand* strand = extractStrandFromCallable(callable);
      if (strand)
      {
        GILScopedUnlock _unlock;
        fut = this->then(strand->schedulerFor(boost::bind(&pyFutureThen, _1, obj))).unwrap();
      }
      else
      {
        GILScopedUnlock _unlock;
        fut = this->then(boost::bind(&pyFutureThen, _1, obj));
      }
      return boost::python::object(PyFuture(fut));
    }

    boost::python::object PyFuture::pyAndThen(
        const boost::python::object& callable)
    {
      if (!PyCallable_Check(callable.ptr()))
        throw std::runtime_error("Not a callable");

      PyThreadSafeObject obj(callable);

      qi::Future<AnyValue> fut;
      qi::Strand* strand = extractStrandFromCallable(callable);
      if (strand)
      {
        GILScopedUnlock _unlock;
        fut = this->andThen(strand->schedulerFor(boost::bind(&pyFutureAndThen, _1, obj))).unwrap();
      }
      else
      {
        GILScopedUnlock _unlock;
        fut = this->andThen(boost::bind(&pyFutureAndThen, _1, obj));
      }
      return boost::python::object(PyFuture(fut));
    }

    boost::python::object PyFuture::unwrap()
    {
      qi::Promise<qi::AnyValue> promise(boost::bind(this->makeCanceler()));
      {
        // why should we unlock here? let me tell you a story...
        // the gil is locked
        // future.connect takes the future's lock
        // and on another thread...
        // something calls setValue on the corresponding promise which takes the same lock
        // setValue copies the value which is in fact a PyThreadSafeObject, and it takes the gil
        // boom deadlock.
        GILScopedUnlock _unlock;
        this->connect(boost::bind(pyFutureUnwrap, _1, promise));
      }
      return boost::python::object(PyFuture(promise.future()));
    }

    namespace
    {
      template<typename Func>
      struct AsFuncPtr
      {
        using MemberPtr = decltype(&Func::operator());
        using Components =
          typename boost::function_types::components<MemberPtr,
                                                     boost::mpl::always<boost::mpl::void_>>::type;
        using type = typename boost::function_types::function_pointer<
          typename boost::mpl::remove<Components, boost::mpl::void_>::type>::type;
      };

      template<typename Func>
      using AsFuncPtrType = typename AsFuncPtr<Func>::type;

      /// Converts a function object with the appropriate cast (potentially user-defined) to a
      /// function pointer.
      // TODO: Replace uses of this when passing a lambda by the unary operator '+' when we don't
      //       support VS2015 anymore.
      template<class Func>
      AsFuncPtrType<Func> asFuncPtr(Func func)
      {
        return static_cast<AsFuncPtrType<Func>>(func);
      }

      boost::shared_ptr<Promise<AnyValue>> makePromise(boost::python::object onCancel)
      {
        boost::function<void(Promise<AnyValue>&)> callback;
        if (!onCancel.is_none())
        {
          PyThreadSafeObject tsOnCancel(onCancel);
          callback = [=](Promise<AnyValue>& prom){
            GILScopedLock lock;
            PY_CATCH_ERROR(tsOnCancel.object()(&prom))
          };
        }

        return boost::make_shared<Promise<AnyValue>>(std::move(callback));
      }
    }

    void export_pyfuture() {
      boost::python::enum_<qi::FutureState>("FutureState")
          .value("None", qi::FutureState_None)
          .value("Running", qi::FutureState_Running)
          .value("Canceled", qi::FutureState_Canceled)
          .value("FinishedWithError", qi::FutureState_FinishedWithError)
          .value("FinishedWithValue", qi::FutureState_FinishedWithValue);


      boost::python::enum_<qi::FutureTimeout>("FutureTimeout")
          .value("None", qi::FutureTimeout_None)
          .value("Infinite", qi::FutureTimeout_Infinite);


      boost::python::class_<Promise<AnyValue>>("Promise", boost::python::no_init)
          .def("__init__",
               boost::python::make_constructor(
                 makePromise,
                 boost::python::default_call_policies(),
                 (boost::python::arg("on_cancel") = boost::python::object())
               ),
               "__init__(on_cancel)\n"
               ":param on_cancel: a function that will be called when a cancel is requested on"
               " the future.\n"
               )

          .def("setCanceled",
               asFuncPtr([](Promise<AnyValue>& prom) {
                 GILScopedUnlock unlock;
                 return prom.setCanceled();
               }),
               "setCanceled() -> None\n"
               "Set the state of the promise to Canceled")

          .def("setError",
               asFuncPtr([](Promise<AnyValue>& prom, const std::string& err) {
                 GILScopedUnlock unlock;
                 return prom.setError(err);
               }),
               "setError(error) -> None\n"
               "Set the error of the promise")


          .def("setValue",
               asFuncPtr([](Promise<AnyValue>& prom, const boost::python::object& value) {
                 qi::AnyValue gvr = qi::AnyValue::from(value);
                 GILScopedUnlock unlock;
                 return prom.setValue(std::move(gvr));
               }),
               "setValue(value) -> None\n"
               "Set the value of the promise")

          .def("future",
               asFuncPtr([](Promise<AnyValue>& prom){
                 return PyFuture(prom.future());
               }),
               "future() -> qi.Future\n"
               "Get a qi.Future from the promise, you can get multiple from the same promise.")

          .def("isCancelRequested",
               asFuncPtr([](Promise<AnyValue>& prom){
                GILScopedUnlock unlock;
                return prom.isCancelRequested();
               }),
               "isCancelRequested() -> bool\n"
               "Return true if the future associated with the promise asked for cancelation");

      boost::python::class_<PyFuture>("Future")
          .def(boost::python::init<boost::python::object>("Initialize the future in the FinishedWithValue state"))

          .def("value", &PyFuture::value, (boost::python::args("timeout") = qi::FutureTimeout_Infinite),
               "value(timeout) -> value\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: the value of the future.\n"
               ":raise: a RuntimeError if the timeout is reached or the future has error.\n"
               "\n"
               "Block until the future is ready.")

          .def("error",
               asFuncPtr([](PyFuture& fut) {
                 GILScopedUnlock unlock;
                 return std::string(fut.error());
               }),
               (boost::python::arg("timeout") = qi::FutureTimeout_Infinite),
               "error(timeout) -> string\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: the error of the future.\n"
               ":raise: a RuntimeError if the timeout is reached or the future has no error.\n"
               "\n"
               "Block until the future is ready.")

          .def("wait",
               asFuncPtr([](PyFuture& fut, int timeout) {
                 GILScopedUnlock unlock;
                 return fut.wait(timeout);
               }),
               (boost::python::arg("timeout") = qi::FutureTimeout_Infinite),
               "wait(timeout) -> qi.FutureState\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: a :py:data:`qi.FutureState`.\n"
               "\n"
               "Wait for the future to be ready.")

          .def("hasError",
               asFuncPtr([](PyFuture& fut, int timeout) {
                 GILScopedUnlock unlock;
                 return fut.hasError(timeout);
               }),
               (boost::python::arg("timeout") = qi::FutureTimeout_Infinite),
               "hasError(timeout) -> bool\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: true if the future has an error.\n"
               ":raise: a RuntimeError if the timeout is reached.\n"
               "\n"
               "Return true or false depending on the future having an error.")

          .def("hasValue",
               asFuncPtr([](PyFuture& fut, int timeout) {
                 GILScopedUnlock unlock;
                 return fut.hasValue(timeout);
               }),
               (boost::python::arg("timeout") = qi::FutureTimeout_Infinite),
               "hasValue(timeout) -> bool\n"
               ":param timeout: a time in milliseconds. Optional.\n"
               ":return: true if the future has a value.\n"
               ":raise: a RuntimeError if the timeout is reached.\n"
               "\n"
               "Return true or false depending on the future having a value.")

          .def("cancel",
               asFuncPtr([](PyFuture& fut) {
                 GILScopedUnlock unlock;
                 return fut.cancel();
               }),
               "cancel() -> None\n"
               "Ask for cancelation.")

          .def("isFinished",
               asFuncPtr([](PyFuture& fut) {
                 GILScopedUnlock unlock;
                 return fut.isFinished();
               }),
               "isFinished() -> bool\n"
               ":return: true if the future is not running anymore (if hasError or hasValue or isCanceled).")

          .def("isRunning",
               asFuncPtr([](PyFuture& fut) {
                 GILScopedUnlock unlock;
                 return fut.isRunning();
               }),
               "isRunning() -> bool\n"
               ":return: true if the future is still running.\n")

          .def("isCanceled",
               asFuncPtr([](PyFuture& fut) {
                 GILScopedUnlock unlock;
                 return fut.isCanceled();
               }),
               "isCanceled() -> bool\n"
               ":return: true if the future is canceled.\n")

          .def("isCancelable", static_cast<bool(*)(PyFuture *)>([](PyFuture *){ return true; }),
               "isCancelable() -> bool\n"
               ":return: always true, all future are cancelable now\n"
               ".. deprecated:: 2.5\n")

          .def("addCallback", &PyFuture::addCallback,
               "addCallback(cb) -> None\n"
               ":param cb: a python callable, could be a method or a function.\n"
               "\n"
               "Add a callback that will be called when the future becomes ready.\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the future itself.")

          .def("then", &PyFuture::pyThen,
               "then(cb) -> None\n"
               ":param cb: a python callable, could be a method or a function.\n"
               ":return: a future that will contain the return value of the callback.\n"
               "\n"
               "Add a callback that will be called when the future becomes ready.\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the future itself.")

          .def("andThen", &PyFuture::pyAndThen,
               "andThen(cb) -> None\n"
               ":param cb: a python callable, could be a method or a function.\n"
               ":return: a future that will contain the return value of the callback.\n"
               "\n"
               "Add a callback that will be called when the future becomes ready if it has a value.\n"
               "If the future finishes with an error, the callback is not called and the future returned by "
               "andThen is set to that error.\n"
               "The callback will be called even if the future is already ready.\n"
               "The first argument of the callback is the value of the future itself.")

          .def("unwrap", &PyFuture::unwrap,
               "unwrap() -> Future\n"
               "\n"
               "If this is a Future of a Future of X, return a Future of X. The state of both futures is forwarded"
               " and cancel requests are forwarded to the appropriate future");

      boost::python::def("futureBarrier", &pyFutureBarrier,
          "futureBarrier(futureList) -> Future\n"
          ":param futureList: A list of Futures to wait for\n"
          ":return: A Future of list of futureList\n"
          "\n"
          "This function returns a future that will be set with all the futures given as argument when they are\n"
          " all finished. This is useful to wait for a bunch of Futures at once."
          );
    }
  }
}
