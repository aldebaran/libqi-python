#include "pystrand.hpp"
#include <qipython/error.hpp>
#include <boost/python.hpp>
#include <qi/strand.hpp>

qiLogCategory("qipy.strand");

namespace qi
{
namespace py
{

bool hasattr(boost::python::object obj, const std::string& attr)
{
  return PyObject_HasAttrString(obj.ptr(), attr.c_str());
}

boost::python::object extractBound(const boost::python::object& obj)
{
  if (hasattr(obj, "im_self"))
    return obj.attr("im_self");
  return boost::python::object();
}

boost::python::object extractPartial(const boost::python::object& obj)
{
  if (!hasattr(obj, "args") || !hasattr(obj, "func"))
    return boost::python::object();

  try
  {
    // is the func an unbound method?
    boost::python::object func(obj.attr("func"));
    boost::python::object self(extractBound(func));
    if (!self.is_none())
      return self;
    // is the func a partial?
    boost::python::object subpartial(extractPartial(func));
    if (!subpartial.is_none())
      return subpartial;
    // is the func a (unbound) method?
    boost::python::object methodtype(
        boost::python::import("types").attr("MethodType"));
    switch (PyObject_IsInstance(func.ptr(), methodtype.ptr()))
    {
    case -1:
      {
        std::string err = PyFormatError();
        qiLogVerbose() << "Error on PyObject_IsInstance: " << err;
      }
      break;
    case 0:
      // not a method, return the object itself
      return func;
    case 1:
      // this is an unbound method, check args
      break;
    }

    boost::python::tuple args(obj.attr("args"));
    if (boost::python::len(args) < 1)
      return boost::python::object();
    return args[0];
  }
  catch (boost::python::error_already_set&)
  {
    return boost::python::object();
  }
}

boost::python::object extractFromCallable(const boost::python::object& obj)
{
  boost::python::object self = extractBound(obj);
  if (!self.is_none())
    return self;
  self = extractPartial(obj);
  if (!self.is_none())
    return self;
  return boost::python::object();
}

qi::Strand* extractStrandFromCallable(const boost::python::object& obj)
{
  boost::python::object self(extractFromCallable(obj));
  if (self.is_none())
    return 0;
  return extractStrandFromObject(self);
}

qi::Strand* extractStrandFromObject(const boost::python::object& obj)
{
  if (hasattr(obj, "__qi_get_strand__"))
  {
    boost::python::object ostrand(obj.attr("__qi_get_strand__")());
    boost::python::extract<qi::Strand&> estrand(ostrand);
    if (estrand.check())
      return &estrand();
  }
  return 0;
}

void export_pystrand()
{
  boost::python::class_<qi::Strand, boost::noncopyable>("Strand",
      boost::python::init<>());
}

}
}
