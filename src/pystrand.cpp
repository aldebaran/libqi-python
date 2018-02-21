#include "pystrand.hpp"
#include <qipython/error.hpp>
#include <qipython/gil.hpp>
#include <boost/python.hpp>

qiLogCategory("qipy.strand");



namespace qi
{
namespace py
{

const char* const objectAttributeStrandName =  "__qi_strand__";
const char* const objectAttributeThreadingName =  "__qi_threading__";
const char* const objectAttributeThreadingValueMulti = "multi";
const char* const objectAttributeImSelfName = "im_self";


namespace {

  boost::python::object strandAttribute(const boost::python::object& object)
  {
    if (qi::py::isMultithreaded(object))
      return {};

    auto strandObject = boost::python::getattr(object,
      qi::py::objectAttributeStrandName, boost::python::object());
    if (!strandObject)
    {
      strandObject = boost::python::object(PyStrand{ new qi::Strand, DeleterPyStrand{} });
      boost::python::setattr(object, qi::py::objectAttributeStrandName, strandObject);
    }
    return strandObject;
  }

}

void DeleterPyStrand::operator()(qi::Strand* strand) const
{
  GILScopedUnlock _;
  strand->join();
  delete strand;
}

bool hasattr(boost::python::object obj, const std::string& attr)
{
  return PyObject_HasAttrString(obj.ptr(), attr.c_str());
}

boost::python::object extractBound(const boost::python::object& obj)
{
  if (hasattr(obj, objectAttributeImSelfName))
    return obj.attr(objectAttributeImSelfName);
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

// TODO: return a safer access to the strand.
// TODO: review/rewrite strand and eventloop accecss and usage which seem dangerous in current code.
qi::Strand* extractStrandFromObject(const boost::python::object& obj)
{
  if (auto strandObject = strandAttribute(obj))
  {
    boost::python::extract<PyStrand&> extractStrand{strandObject};
    if (extractStrand.check())
    {
      return extractStrand().get();
    }
  }
  return nullptr;
}


bool isMultithreaded(const boost::python::object& obj)
{
  // An object is multithreaded only if it is declared as such. In any other case it is, either
  // explicitly or implicitly, singlethreaded.
  const auto pyqisig =
      boost::python::getattr(obj, objectAttributeThreadingName, boost::python::object());
  return pyqisig && boost::python::extract<std::string>(pyqisig)() == objectAttributeThreadingValueMulti;
}

void export_pystrand()
{
  boost::python::class_<PyStrand>("Strand",
      boost::python::init<>());
}

}
}
