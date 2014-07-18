/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyproperty.hpp>
#include <qipython/pythreadsafeobject.hpp>
#include <qipython/error.hpp>
#include <boost/python.hpp>
#include <qi/property.hpp>
#include <qi/anyobject.hpp>

namespace qi { namespace py {

    void pyPropertyCb(const qi::AnyValue& val, const PyThreadSafeObject& callable) {
      GILScopedLock _lock;
      PY_CATCH_ERROR(callable.object()(val.to<boost::python::object>()));
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

      ~PyProperty() {
      }

      boost::python::object val() const {
        return value().to<boost::python::object>();
      }

      //change the name to avoid warning "hidden overload in base class" : YES WE KNOW :)
      void setVal(boost::python::object obj) {
        qi::GenericProperty::setValue(obj);
      }

      void addCallback(const boost::python::object &callable) {
        PyThreadSafeObject obj(callable);
        if (!PyCallable_Check(callable.ptr()))
          throw std::runtime_error("Not a callable");
        {
          GILScopedUnlock _unlock;
          connect(boost::bind<void>(&pyPropertyCb, _1, obj));
        }
      }
    };

    class PyProxyProperty {
    public:
      PyProxyProperty(qi::AnyObject obj, const qi::MetaProperty &signal)
        : _obj(obj)
        , _sigid(signal.uid()){
      }

      //TODO: support async
      boost::python::object value() const {
        return _obj.property(_sigid).value().to<boost::python::object>();
      }

      //TODO: support async
      void setValue(boost::python::object obj) {
        _obj.setProperty(_sigid, qi::AnyValue::from(obj));
      }

      void addCallback(const boost::python::object &callable) {
        PyThreadSafeObject obj(callable);
        if (!PyCallable_Check(callable.ptr()))
          throw std::runtime_error("Not a callable");
        {
          GILScopedUnlock _unlock;
          _obj.connect(_sigid, AnyFunction::from(boost::bind<void>(&pyPropertyCb, _1, obj)));
        }
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

          .def("addCallback", &PyProperty::addCallback,
               "addCallback(cb) -> None\n"
               ":param cb: the callback to call when the property changes\n"
               "\n"
               "add a callback to the property");

      boost::python::class_<PyProxyProperty>("_ProxyProperty", boost::python::no_init)
          .def("value", &PyProxyProperty::value)
          .def("setValue", &PyProxyProperty::setValue, (boost::python::arg("value")))
          .def("addCallback", &PyProxyProperty::addCallback);
    }

  }
}
