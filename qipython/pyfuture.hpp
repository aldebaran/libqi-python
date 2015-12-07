#pragma once
/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QIPYTHON_PYFUTURE_HPP_
#define _QIPYTHON_PYFUTURE_HPP_

// We need this header because on macOS python redefine macro of string operations (isalnum, isspace...)
// this redefintion conflict with ios definition of those function.
// for more information take a look to python2.7/pyport.h headers under _PY_PORT_CTYPE_UTF8_ISSUE section.
#include <locale>

#include <boost/python.hpp>
#include <qi/future.hpp>
#include <qi/anyvalue.hpp>
#include <qipython/gil.hpp>
#include <qipython/api.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

namespace qi {
  namespace py {

    class PyPromise;
    class PyThreadSafeObject;

    class PyFuture : public qi::Future<qi::AnyValue> {
      friend class PyPromise;
      friend class PyFutureBarrier;
      friend void pyFutureCb(const qi::Future<qi::AnyValue>& fut, const PyThreadSafeObject& callable);
      friend qi::AnyValue pyFutureThen(const qi::Future<qi::AnyValue>& fut, const PyThreadSafeObject& callable);
      friend void onBarrierFinished(const std::vector<qi::Future<qi::AnyValue> >& futs, PyPromise prom);

    public:
      PyFuture();
      PyFuture(const boost::python::object& obj);
      PyFuture(const qi::Future<qi::AnyValue>& fut);
      boost::python::object value(int msecs = qi::FutureTimeout_Infinite) const;
      std::string error(int msecs = qi::FutureTimeout_Infinite) const;
      void addCallback(const boost::python::object &callable);
      boost::python::object pyThen(const boost::python::object& callable);
      boost::python::object pyAndThen(const boost::python::object& callable);
      boost::python::object unwrap();
      FutureState wait(int msecs) const;
      bool        hasError(int msecs) const;
      bool        hasValue(int msecs) const;
      void        cancel();
    };


    class PyPromise: public qi::Promise<qi::AnyValue> {
    public:
      PyPromise();
      PyPromise(const qi::Promise<qi::AnyValue> &ref);
      PyPromise(boost::python::object callable);
      void setValue(const boost::python::object &pyval);
      PyFuture future();
    };


    //convert from Future to PyFuture
    template <typename T>
    PyFuture toPyFuture(qi::Future<T> fut) {
      PyPromise gprom;
      qi::adaptFuture(fut, gprom);
      return gprom.future();
    }

    //convert from FutureSync to PyFuture
    template <typename T>
    PyFuture toPyFuture(qi::FutureSync<T> fut) {
      return toPyFuture(fut.async());
    }

    //async == true  => convert to PyFuture
    //async == false => convert to PyObject or throw on error
    template <typename T>
    boost::python::object toPyFutureAsync(qi::Future<T> fut, bool async) {
      if (async)
        return boost::python::object(toPyFuture(fut));
      {
        GILScopedUnlock _;
        fut.wait();
      }
      return qi::AnyReference::from(fut.value()).template to<boost::python::object>(); //throw on error
    }

    template <>
    inline boost::python::object toPyFutureAsync<void>(qi::Future<void> fut, bool async) {
      if (async)
        return boost::python::object(toPyFuture(fut));
      {
        GILScopedUnlock _;
        fut.value(); //wait for the result
      }
      return boost::python::object(); //throw on error
    }

    template <typename T>
    boost::python::object toPyFutureAsync(qi::FutureSync<T> fut, bool async) {
      return toPyFutureAsync(fut.async(), async);
    }

    void export_pyfuture();

  }
}


#endif  // _QIPYTHON_PYFUTURE_HPP_
