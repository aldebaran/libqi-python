/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/
#include <boost/python.hpp>
#include <map>

#include <qi/anyobject.hpp>
#include <qi/anymodule.hpp>
#include <qipython/gil.hpp>
#include <qipython/pymodule.hpp>
#include <qipython/pyobject.hpp>
#include <qipython/error.hpp>
#include <boost/python/raw_function.hpp>

qiLogCategory("qipy.factory");

namespace qi {
  namespace py {
    class PyCreateException : std::exception
    {
    public:
      PyCreateException(const std::string &name) : _msg(name + "not found") {}
      virtual ~PyCreateException() throw() {}
      char const * what() const throw() { return _msg.c_str(); }

    private:
      std::string _msg;
    };

    void translate_pycreateexception(PyCreateException const& e)
    {
      PyErr_SetString(PyExc_RuntimeError, e.what());
    }

//    static qi::AnyObject pyconstruct_object(boost::python::object class_)
//    {
//      GILScopedLock lock;
//      qi::AnyObject obj;
//      PY_CATCH_ERROR(obj = makeQiAnyObject(class_()));
//      return obj;
//    }

//    static void pyregister_object_factory(boost::python::str class_name, boost::python::object class_)
//    {
//      std::string name = boost::python::extract<std::string>(class_name);
//      boost::function<qi::AnyObject()> func = boost::bind(&qi::py::pyconstruct_object, class_);
//      qi::registerObjectFactory(name, AnyFunction::from(func));
//    }

//    static boost::python::object pycreate_object_args(boost::python::tuple pyargs,
//                                                      boost::python::dict kwargs)
//    {
//      int           len        = boost::python::len(pyargs);
//      std::string   objectName = boost::python::extract<std::string>(pyargs[0]);
//      qi::AnyObject object;

//      qi::AnyReferenceVector args;
//      for (int i = 1; i < len; ++i)
//        args.push_back(AnyReference::from(boost::python::object(pyargs[i])));
//      object = qi::createObject(objectName, args);

//      if(!object)
//        throw PyCreateException(objectName);
//      return makePyQiObject(object, objectName);
//    }

    class PyModule {
    public:
      explicit PyModule(const qi::AnyModule& mod)
       : _mod(mod)
      {}

      PyModule()
      {}

      qi::AnyModule _mod;
    };

    static boost::python::object createObjectAdapter(boost::python::tuple pyargs,
                                                     boost::python::dict kwargs)
    {
      int           len        = boost::python::len(pyargs);
      PyModule      mod = boost::python::extract<PyModule>(pyargs[0]);
      std::string   objectName = boost::python::extract<std::string>(pyargs[1]);
      qi::AnyObject object;

      qi::AnyReferenceVector args;
      for (int i = 1; i < len; ++i)
        args.push_back(AnyReference::from(boost::python::object(pyargs[i])));
      //TODO object = mod._mod->createObject(objectName, args);

      if(!object)
        throw PyCreateException(objectName);
      return makePyQiObject(object, objectName);
    }



    static boost::python::object pyqimodule(const boost::python::str& modname) {
      std::string name = boost::python::extract<std::string>(modname);
      return boost::python::object(PyModule(qi::import(name)));
    }

    static boost::python::list pylistModules()
    {
      std::vector<qi::ModuleInfo> vect = qi::listModules();
      return qi::AnyReference::from(vect).to<boost::python::list>();
    }

    // Python AnyModule Factory
    qi::AnyModule importPyModule(const qi::ModuleInfo& name) {
      qiLogInfo() << "import in python not implemented yet";
      return qi::AnyModule();
    }
    QI_REGISTER_MODULE_FACTORY("python", &importPyModule);


    void export_pyobjectfactory()
    {
      boost::python::register_exception_translator<PyCreateException>(&translate_pycreateexception);
      boost::python::def("module", &pyqimodule);

      boost::python::def("listModules", &pylistModules);
      boost::python::class_<PyModule>("Module", boost::python::init<>())
          .def("createObject", boost::python::raw_function(&createObjectAdapter, 2))
//          .def("constants", &PyModule::constants)
//          .def("functions", &PyModule::functions)
//          .def("factories", &PyModule::factories)
          ;

      //boost::python::def("createObject", boost::python::raw_function(&pycreate_object_args, 1));
      //boost::python::def("registerObjectFactory", &qi::py::pyregister_object_factory);
    }
  }
}
