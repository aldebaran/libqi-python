/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyproperty.hpp>
#include <qipython/pythreadsafeobject.hpp>
#include <qipython/error.hpp>
#include <qipython/pyfuture.hpp>
#include <boost/python.hpp>
#include <qi/property.hpp>
#include <qi/anyobject.hpp>
#include "pystrand.hpp"

namespace qi { namespace py {
    qi::AnyFunction makePyPropertyCb(const PyThreadSafeObject& callable)
    {
      return qi::AnyFunction::from([callable](const qi::AnyValue& value)
      {
        GILScopedLock lock;
        PY_CATCH_ERROR(callable.object()(value.to<boost::python::object>()));
      });
    }

    class PyProperty : public qi::GenericProperty {
    public:
      PyProperty()
        : qi::GenericProperty(qi::TypeInterface::fromSignature("m"))
      {
      }

      explicit PyProperty(const std::string &signature)
        : qi::GenericProperty(qi::TypeInterface::fromSignature(signature))
      {
      }

      ~PyProperty()
      {
        //the dtor can lock waiting for callback ends
        GILScopedUnlock _unlock;
        this->disconnectAll();
      }

      boost::python::object val() const
      {
        qi::Future<qi::AnyValue> f;
        {
          GILScopedUnlock _unlock;
          f = qi::GenericProperty::value();
          f.wait();
        }
        return f.value().to<boost::python::object>();
      }

      //change the name to avoid warning "hidden overload in base class" : YES WE KNOW :)
      void setVal(boost::python::object obj)
      {
        GILScopedUnlock _;
        qi::GenericProperty::setValue(obj);
      }

      boost::python::object addCallback(const boost::python::object& callable, bool _async = false) {
        qi::uint64_t link;
        if (!PyCallable_Check(callable.ptr()))
          throw std::runtime_error("Not a callable");

        PyThreadSafeObject obj(callable);

        qi::Strand* strand = extractStrandFromCallable(callable);
        if (strand)
        {
          GILScopedUnlock _unlock;
          link = connect(SignalSubscriber{makePyPropertyCb(obj), strand});
        }
        else
        {
          GILScopedUnlock _unlock;
          link = connect(makePyPropertyCb(obj));
        }

        if (_async)
        {
          return boost::python::object(toPyFuture(qi::Future<qi::uint64_t>(link)));
        }
        return boost::python::object(link);
      }

      boost::python::object disc(qi::uint64_t id, bool _async = false) {
        bool r;
        {
          GILScopedUnlock _unlock;
          r = disconnect(id);
        }
        if (_async)
        {
          return boost::python::object(toPyFuture(qi::Future<bool>(r)));
        }

        return boost::python::object(r);
      }

      boost::python::object discAll(bool _async = false) {
        bool r;
        {
          GILScopedUnlock _unlock;
          r = disconnectAll();
        }
        if (_async)
        {
          return boost::python::object(toPyFuture(qi::Future<qi::uint64_t>(r)));
        }
        return boost::python::object(r);
      }
    };

    class PyProxyProperty {
    public:
      PyProxyProperty(qi::AnyObject obj, const qi::MetaProperty &signal)
        : _obj(obj)
        , _sigid(signal.uid())
      {
      }

      ~PyProxyProperty()
      {
        //the dtor can lock waiting for callback ends
        GILScopedUnlock _unlock;
        _obj.reset();
      }

      boost::python::object value(bool _async = false) const {
        qi::Future<AnyValue> f;
        {
          GILScopedUnlock _unlock;
          f = _obj.property(_sigid);
        }
        return toPyFutureAsync(f, _async);
      }

      boost::python::object setValue(boost::python::object obj, bool _async = false) {
        qi::Future<void> f;
        {
          GILScopedUnlock _unlock;
          f = _obj.setProperty(_sigid, qi::AnyValue::from(obj));
        }
        return toPyFutureAsync(f, _async);
      }

      boost::python::object addCallback(const boost::python::object &callable, bool _async = false) {
        PyThreadSafeObject obj(callable);
        if (!PyCallable_Check(callable.ptr()))
          throw std::runtime_error("Not a callable");

        qi::Future<SignalLink> f;
        qi::Strand* strand = extractStrandFromCallable(callable);
        if (strand)
        {
          GILScopedUnlock _unlock;
          f = _obj.connect(_sigid, SignalSubscriber{makePyPropertyCb(obj), strand});
        }
        else
        {
          GILScopedUnlock _unlock;
          f = _obj.connect(_sigid, makePyPropertyCb(obj));
        }
        return toPyFutureAsync(f, _async);
      }

      boost::python::object disc(qi::uint64_t id, bool _async = false) {
        qi::Future<void> f;
        {
          GILScopedUnlock _unlock;
          f = _obj.disconnect(id);
        }
        return toPyFutureAsync(f, _async);
      }

    private:
      qi::AnyObject _obj;
      unsigned int  _sigid;
    };

    boost::python::object makePyProperty(const std::string &signature) {
      return boost::python::object(boost::make_shared<PyProperty>(signature));
    }

    qi::PropertyBase *getProperty(boost::python::object obj) {
      boost::shared_ptr<PyProperty> p = boost::python::extract< boost::shared_ptr<PyProperty> >(obj);
      if (!p)
        return 0;
      return p.get();
    }

    boost::python::object makePyProxyProperty(const qi::AnyObject &obj, const qi::MetaProperty &prop) {
      return boost::python::object(PyProxyProperty(obj, prop));
    }

    void export_pyproperty() {
      //use a shared_ptr because class Property is not copyable.
      boost::python::class_<PyProperty, boost::shared_ptr<PyProperty>, boost::noncopyable >("Property", boost::python::init<>())
          .def(boost::python::init<const std::string &>())
          .def("value", &PyProperty::val,
               "value() -> value\n"
               ":return: the value stored inside the property")

          .def("setValue", &PyProperty::setVal, (boost::python::arg("value")),
               "setValue(value) -> None\n"
               ":param value: the value of the property\n"
               "\n"
               "set the value of the property")

          .def("addCallback", &PyProperty::addCallback, (boost::python::arg("cb"), boost::python::arg("_async") = false),
               "addCallback(cb) -> int\n"
               ":param cb: the callback to call when the property changes\n"
               "\n"
               "add a callback to the property")

          .def("connect", &PyProperty::addCallback, (boost::python::arg("cb"), boost::python::arg("_async") = false),
               "connect(cb) -> int\n"
               ":param cb: the callback to call when the property changes\n"
               "\n"
               "add a callback to the property")

          .def("disconnect", &PyProperty::disc, (boost::python::arg("id"), boost::python::arg("_async") = false),
               "disconnect(id) -> bool\n"
               ":param id: the connection id returned by connect\n"
               ":return: true on success\n"
               "\n"
               "Disconnect the callback associated to id.")

          .def("disconnectAll", &PyProperty::disconnectAll, (boost::python::arg("_async") = false),
               "disconnectAll() -> bool\n"
               ":return: true on success\n"
               "\n"
               "disconnect all callback associated to the signal. This function should be used very carefully. It's extremely rare that it is needed.");

      boost::python::class_<PyProxyProperty>("_ProxyProperty", boost::python::no_init)
          .def("value", &PyProxyProperty::value, (boost::python::arg("_async") = false))
          .def("setValue", &PyProxyProperty::setValue, (boost::python::arg("value"), boost::python::arg("_async") = false))
          .def("addCallback", &PyProxyProperty::addCallback, (boost::python::arg("cb"), boost::python::arg("_async") = false))
          .def("connect", &PyProxyProperty::addCallback, (boost::python::arg("cb"), boost::python::arg("_async") = false))
          .def("disconnect", &PyProxyProperty::disc, (boost::python::arg("id"), boost::python::arg("_async") = false));
    }
  }
}

namespace boost
{
    template <>
    qi::py::PyProperty const volatile * get_pointer<class qi::py::PyProperty const volatile >(
        class qi::py::PyProperty const volatile *c)
        {
            return c;
        }
}
