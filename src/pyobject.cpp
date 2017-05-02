/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <qipython/pyobject.hpp>
#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pysignal.hpp>
#include <qipython/pyproperty.hpp>
#include <qipython/error.hpp>
#include <qipython/pythreadsafeobject.hpp>
#include <qi/type/dynamicobjectbuilder.hpp>
#include <qi/type/objecttypebuilder.hpp> //for TYPE_TEMPLATE(Future)
#include <qi/jsoncodec.hpp>
#include <qi/strand.hpp>
#include <qipython/gil.hpp>

#include "pystrand.hpp"
#include "pyobject_p.hpp"

qiLogCategory("qipy.object");

namespace qi { namespace py {

    PyQiFunctor::~PyQiFunctor()
    {
        qi::py::GILScopedUnlock _lock;
        _object.reset();
    }

    PyQiObject::~PyQiObject()
    {
        qi::py::GILScopedUnlock _lock;
        _object.reset();
    }

    boost::python::object PyQiFunctor::operator()(boost::python::tuple pyargs, boost::python::dict pykwargs) {
      qi::AnyValue val = qi::AnyValue::from(pyargs);
      bool async = boost::python::extract<bool>(pykwargs.get("_async", false));
      std::string funN = boost::python::extract<std::string>(pykwargs.get("_overload", _funName));
      // TODO: make it so the message under does not need to be uncommented
      //        qiLogDebug() << "calling a method: " << funN << " args:" << qi::encodeJSON(val);

      qi::Future<qi::AnyValue> fut;
      qi::Promise<qi::AnyValue> res;
      PyPromise pyprom(res);
      {
        //calling c++, so release the GIL.
        GILScopedUnlock _unlock;
        qi::Future<qi::AnyReference> fmeta = _object.metaCall(funN, val.content().asTupleValuePtr(), async ? MetaCallType_Queued : MetaCallType_Direct);
        //because futureAdapter support AnyRef containing Future<T>  (that will be converted to a Future<T>
        // instead of Future<Future<T>>
        adaptFutureUnwrap(fmeta, res);
        if (!async)
          return res.future().value().to<boost::python::object>();
      }
      return boost::python::object(pyprom.future());
    }

    boost::python::object importInspect() {
      static bool plouf = false;
      static boost::python::object& obj = *new boost::python::object;
      if (!plouf) {
        obj = boost::python::import("inspect");
        plouf = true;
      }
      return obj;
    }


    void populateMethods(boost::python::object pyobj, qi::AnyObject obj) {
      qi::MetaObject::MethodMap           mm = obj.metaObject().methodMap();
      qi::MetaObject::MethodMap::iterator it;
      for (it = mm.begin(); it != mm.end(); ++it) {
        qi::MetaMethod &mem = it->second;
        //drop special methods
        if (mem.uid() < qiObjectSpecialMemberMaxUid)
          continue;
        qiLogDebug() << "adding method:" << mem.toString();
        boost::python::object fun = boost::python::raw_function(PyQiFunctor(mem.name(), obj));
        boost::python::api::setattr(pyobj, mem.name().c_str(), fun);
        // Fill __doc__ with Signature and description
        std::stringstream ssdocstring;
        ssdocstring << "Signature: " << mem.returnSignature().toString() << "\n";
        ssdocstring << mem.description();
        boost::python::object docstring(ssdocstring.str());
        boost::python::api::setattr(fun, "__doc__", docstring);
      }
    }

    void populateSignals(boost::python::object pyobj, qi::AnyObject obj) {
      qi::MetaObject::SignalMap           mm = obj.metaObject().signalMap();
      qi::MetaObject::SignalMap::iterator it;
      for (it = mm.begin(); it != mm.end(); ++it) {
        qi::MetaSignal &ms = it->second;
        //drop special methods
        if (ms.uid() < qiObjectSpecialMemberMaxUid)
          continue;
        qiLogDebug() << "adding signal:" << ms.toString();
        boost::python::object fun = qi::py::makePyProxySignal(obj, ms);
        boost::python::api::setattr(pyobj, ms.name(), fun);
      }
    }

    void populateProperties(boost::python::object pyobj, qi::AnyObject obj) {
      qi::MetaObject::PropertyMap           mm = obj.metaObject().propertyMap();
      qi::MetaObject::PropertyMap::iterator it;
      for (it = mm.begin(); it != mm.end(); ++it) {
        qi::MetaProperty &mp = it->second;
        //drop special methods
        if (mp.uid() < qiObjectSpecialMemberMaxUid)
          continue;
        qiLogDebug() << "adding property:" << mp.toString();
        boost::python::object fun = qi::py::makePyProxyProperty(obj, mp);
        boost::python::api::setattr(pyobj, mp.name().c_str(), fun);
      }
    }




    boost::python::object makePyQiObject(qi::AnyObject obj, const std::string &name) {
      if (QI_TEMPLATE_TYPE_GET(obj.asGenericObject()->type, Future) ||
          QI_TEMPLATE_TYPE_GET(obj.asGenericObject()->type, FutureSync))
        return boost::python::object(PyFuture(obj.async<qi::AnyValue>("_getSelf")));

      boost::python::object result = boost::python::object(qi::py::PyQiObject(obj));
      qi::py::populateMethods(result, obj);
      qi::py::populateSignals(result, obj);
      qi::py::populateProperties(result, obj);
      return result;
    }


    //TODO: DO NOT DUPLICATE
    static qi::AnyReference pyCallMethod(const std::vector<qi::AnyReference>& cargs, PyThreadSafeObject tscallable) {
      qi::AnyReference gvret;
      try {
        qi::py::GILScopedLock _lock;
        boost::python::list   args;
        boost::python::object ret;
        boost::python::object callable = tscallable.object();

        std::vector<qi::AnyReference>::const_iterator it = cargs.begin();
        ++it; //drop the first arg which is DynamicObject*
        for (; it != cargs.end(); ++it) {
          qiLogDebug() << "argument: " << qi::encodeJSON(*it);
          args.append(it->to<boost::python::object>());
        }
        qiLogDebug() << "before method call";
        try {
          ret = callable(*boost::python::tuple(args));
        } catch (const boost::python::error_already_set &e) {
          std::string err = PyFormatError();
          qiLogDebug() << "call exception:" << err;
          throw std::runtime_error(err);
        }

        //convert python future to future, to allow the middleware to make magic with it.
        //serverresult will support async return value for call. (call returning future)
        boost::python::extract< PyFuture* > extractor(ret);
        if (extractor.check()) {
          PyFuture* pfut = extractor();
          if (pfut) { //pfut == 0, can mean ret is None.
            qiLogDebug() << "Future detected";
            qi::Future<qi::AnyValue> fut = *pfut;
            return qi::AnyReference::from(fut).clone();
          }
        }

        gvret = qi::AnyReference::from(ret).clone();
        qiLogDebug() << "method returned:" << qi::encodeJSON(gvret);
      } catch (const boost::python::error_already_set &e) {
        qi::py::GILScopedLock _lock;
        throw std::runtime_error("Python error: " + PyFormatError());
      }
      return gvret;
    }

    //callback just needed to keep a ref on obj
    //Use PyThreadSafeObject to be GILSafe
    static void doNothing(qi::GenericObject *go, PyThreadSafeObject obj)
    {
      (void)go;
      (void)obj;
    }

    //get the signature for the function
    //if vargs => return m
    //else: return (m...m) with the good number of m
    std::string generateDefaultParamSignature(const std::string &key, boost::python::object& argspec, bool isMeth)
    {
      //argspec[0] = args
      int argsz = boost::python::len(argspec[0]);

      //argspec[1] = name of vargs
      if (argspec[1])
      {
        //m is accept everything
        return "m";
      }

      if (argsz == 0 && isMeth) {
        std::stringstream serr;
        serr << "Method " << key << " is missing the self argument.";
        throw std::runtime_error(serr.str());
      }

      //drop self on method
      if (isMeth)
        argsz = argsz - 1;

      std::stringstream ss;

      ss << "(";
      for (int i = 0; i < argsz; ++i)
        ss << "m";
      ss << ")";
      return ss.str();
    }

    void registerMethod(qi::DynamicObjectBuilder& gob, const std::string& key, boost::python::object& method, const std::string& qisig)
    {
      if (boost::starts_with(key, "__")) {
        qiLogVerbose() << "Not binding private method: " << key;
        return;
      }
      qi::MetaMethodBuilder mmb;
      mmb.setName(key);
      boost::python::object desc = method.attr("__doc__");
      boost::python::object pyqiretsig = boost::python::getattr(method, "__qi_return_signature__", boost::python::object());
      if (desc)
        mmb.setDescription(boost::python::extract<std::string>(desc));
      boost::python::object inspect = importInspect();
      boost::python::object tu;
      try {
        //returns: (args, varargs, keywords, defaults)
        tu = inspect.attr("getargspec")(method);
      } catch(const boost::python::error_already_set& e) {
        std::string s = PyFormatError();
        qiLogWarning() << "Skipping the registration of '" << key << "': " << s;
        return;
      }

      std::string defparamsig = generateDefaultParamSignature(key, tu, PyMethod_Check(method.ptr()));

      qiLogDebug() << "Adding method: " << defparamsig;
      if (!qisig.empty())
        mmb.setParametersSignature(qisig);
      else
        mmb.setParametersSignature(defparamsig);

      std::string qiretsig;
      if (pyqiretsig) {
        qiretsig = boost::python::extract<std::string>(pyqiretsig);
      }

      if (!qiretsig.empty())
        mmb.setReturnSignature(qiretsig);
      else
        mmb.setReturnSignature("m");

      // Throw on error
      gob.xAdvertiseMethod(mmb, qi::AnyFunction::fromDynamicFunction(boost::bind(pyCallMethod, _1, PyThreadSafeObject(method))));
    }

    ObjectThreadingModel getThreadingModel(boost::python::object &obj) {
      boost::python::object pyqisig = boost::python::getattr(obj, "__qi_threading__", boost::python::object());
      std::string qisig;
      if (pyqisig)
        qisig = boost::python::extract<std::string>(pyqisig);
      if (qisig == "multi")
        return ObjectThreadingModel_MultiThread;
      else if (qisig == "single")
        return ObjectThreadingModel_SingleThread;
      return ObjectThreadingModel_SingleThread;
    }

    static void deletenoop(qi::Strand*)
    {}

    qi::AnyObject makeQiAnyObject(boost::python::object obj)
    {
      {
        //is that a qi::AnyObject?
        boost::python::extract<PyQiObject*> isthatyoumum(obj);

        if (isthatyoumum.check()) {
          qiLogDebug() << "This PyObject is already a qi::AnyObject. Just returning it.";
          return isthatyoumum()->object();
        }
      }
      {
        //is that a qi::Future?
        boost::python::extract<PyFuture*> isthatyoumum(obj);

        if (isthatyoumum.check()) {
          qiLogDebug() << "This PyObject is a Future. Making a qi::Object from it.";
          return qi::AnyObject(boost::make_shared<qi::Future<qi::AnyValue> >(*isthatyoumum()));
        }
      }

      qi::DynamicObjectBuilder gob;

      //let the GIL handle the thread-safety for us
      gob.setThreadingModel(getThreadingModel(obj));
      GILScopedLock _lock;
      boost::python::object attrs(boost::python::borrowed(PyObject_Dir(obj.ptr())));

      boost::python::object asignal = qi::py::makePySignal("(i)").attr("__class__");
      boost::python::object aproperty = qi::py::makePyProperty("(i)").attr("__class__");

      for (int i = 0; i < boost::python::len(attrs); ++i) {
        std::string key = boost::python::extract<std::string>(attrs[i]);
        boost::python::object m = obj.attr(attrs[i]);

        boost::python::object pyqiname = boost::python::getattr(m, "__qi_name__", boost::python::object());
        boost::python::object pyqisig = boost::python::getattr(m, "__qi_signature__", boost::python::object());
        std::string qisig;

        if (pyqisig) {
          boost::python::extract<std::string> ex(pyqisig);

          if (!ex.check())
            qiLogWarning() << "__qi_signature__ for " << key << " is not of type string";
          else
            qisig = ex();
        }

        if (qisig == "DONOTBIND")
          continue;
        //override name by the one provide by @bind
        if (pyqiname) {
          boost::python::extract<std::string> ex(pyqiname);
          if (!ex.check())
            qiLogWarning() << "__qi_name__ for " << key << " is not of type string";
          else
            key = ex();
        }

        //Handle Signal
        if (PyObject_IsInstance(m.ptr(), asignal.ptr())) {
          qiLogDebug() << "Adding signal:" << key;
          gob.advertiseSignal(key, qi::py::getSignal(m));
          continue;
        }

        //Handle Property
        if (PyObject_IsInstance(m.ptr(), aproperty.ptr())) {
          qiLogDebug() << "Adding property:" << key;
          gob.advertiseProperty(key, qi::py::getProperty(m));
          continue;
        }

        //Handle Methods, Functions, ... everything that is callable.
        if (PyCallable_Check(m.ptr())) {
          registerMethod(gob, key, m, qisig);

          continue;
        }
      }
      //this is a useless callback, needed to keep a ref on obj.
      //when the GO is destructed, the ref is released.
      //doNothing use PyThreadSafeObject to lock the GIL as needed
      qi::AnyObject anyobj = gob.object(boost::bind<void>(&doNothing, _1, obj));

      if (qi::Strand* strand = extractStrandFromObject(obj))
        // no need to keep the strand alive because the anyobject is already doing that
        anyobj.forceExecutionContext(boost::shared_ptr<qi::Strand>(strand, deletenoop));

      return anyobj;
    }

    void export_pyobject() {
      boost::python::class_<qi::py::PyQiObject>("Object", boost::python::no_init)
          .def("call", boost::python::raw_function(&pyParamShrinker<PyQiObject>, 1))
          .def("async", boost::python::raw_function(&pyParamShrinkerAsync<PyQiObject>, 1))
          //TODO: .def("post")
          //TODO: .def("setProperty")
          //TODO: .def("property")
          .def("metaObject", &qi::py::PyQiObject::metaObject);
      //import inspect in our current namespace
      importInspect();
    }
  }
}
