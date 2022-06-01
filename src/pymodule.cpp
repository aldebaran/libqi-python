/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/common.hpp>
#include <qipython/pymodule.hpp>
#include <qipython/pyobject.hpp>
#include <qipython/pyguard.hpp>
#include <qi/anyobject.hpp>
#include <qi/anymodule.hpp>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

::py::object call(::py::object obj, ::py::str name,
                  ::py::args args, ::py::kwargs kwargs)
{
  GILAcquire lock;
  return obj.attr(name)(*args, **kwargs);
}

::py::object getModule(const std::string& name)
{
  const auto mod = import(name);
  const auto pyMod = toPyObject(mod);
  const ::py::cpp_function callFn(&call, ::py::is_method(pyMod.get_type()),
                                  ::py::arg("name"));

  const auto types = ::py::module::import("types");
  ::py::setattr(pyMod, "createObject", types.attr("MethodType")(callFn, pyMod));
  return pyMod;
}

::py::list listModules()
{
  const auto modules = invokeGuarded<GILRelease>(&qi::listModules);
  return castToPyObject(AnyReference::from(modules));
}

} // namespace

void exportObjectFactory(::py::module& m)
{
  using namespace ::py;

  GILAcquire lock;

  m.def("module", &getModule,
        doc(":returns: an object that represents the requested module.\n"));
  m.def("listModules", &listModules);
}

} // namespace py
} // namespace qi
