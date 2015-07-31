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

#include "pyobject_p.hpp"

namespace bpy = boost::python;

qiLogCategory("qipy.module");

namespace qi
{
namespace py
{
  static bpy::object createObjectAdapter(bpy::tuple pyargs, bpy::dict kwargs)
  {
    bpy::object pymod = pyargs[0];
    bpy::str name(pyargs[1]);
    bpy::list args(pyargs.slice(2, bpy::len(pyargs)));
    return pymod.attr(name)(*args, **kwargs);
  }

  static bpy::object pyqimodule(const std::string& name)
  {
    qi::AnyModule mod = qi::import(name);
    bpy::object pymod = makePyQiObject(mod);
    bpy::object createFun = bpy::raw_function(createObjectAdapter);

    bpy::object types = bpy::import("types");
    bpy::api::setattr(pymod, "createObject", types.attr("MethodType")(createFun, pymod));
    return pymod;
  }

  static bpy::list pylistModules()
  {
    std::vector<qi::ModuleInfo> vect = qi::listModules();
    return qi::AnyReference::from(vect).to<bpy::list>();
  }

  // Python AnyModule Factory
  qi::AnyModule importPyModule(const qi::ModuleInfo& name)
  {
    qiLogInfo() << "import in python not implemented yet";
    return qi::AnyModule();
  }
  QI_REGISTER_MODULE_FACTORY("python", &importPyModule);

  void export_pyobjectfactory()
  {
    bpy::def("module", &pyqimodule,
        "module(moduleName) -> object\n"
        ":return: an object that represents the requested module\n");
    bpy::def("listModules", &pylistModules);
  }
}
}
