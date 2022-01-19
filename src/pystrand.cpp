/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pystrand.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <pybind11/pybind11.h>

qiLogCategory("qi.python.strand");

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

const char* const objectAttributeStrandName =  "__qi_strand__";
const char* const objectAttributeThreadingName =  "__qi_threading__";
const char* const objectAttributeThreadingValueMulti = "multi";

// Returns true if `func` is an unbound method of `object`, by checking if one
// of the attribute of `object` is `func`.
//
// @pre `func` and `object` are not null objects.
bool isUnboundMethod(const ::py::function& func, const ::py::object& object)
{
  QI_ASSERT_TRUE(func);
  QI_ASSERT_TRUE(object);

  GILAcquire lock;
  const auto dir = ::py::reinterpret_steal<::py::list>(PyObject_Dir(object.ptr()));
  for (const ::py::handle attrName : dir)
  {
    const ::py::handle attr = object.attr(attrName);
    QI_ASSERT_TRUE(attr);

    if (!PyMethod_Check(attr.ptr()))
      continue;

    const ::py::handle unboundMethod = PyMethod_GET_FUNCTION(attr.ptr());
    if (func.is(unboundMethod))
      return true;
  }
  return false;
}

// Returns the instance associated with a partial application of a method, if
// passed.
//
// If no instance is associated, or if the object is not a partial application,
// returns `None`.
//
// See https://docs.python.org/3/library/functools.html for more details about
// partial objects.
//
// They have three read-only attributes:
//   - partial.func
//     A callable object or function. Calls to the partial object will be
//     forwarded to func with new arguments and keywords.
//
//   - partial.args
//     The leftmost positional arguments that will be prepended to the
//     positional arguments provided to a partial object call.
//
//   - partial.keywords
//     The keyword arguments that will be supplied when the partial object is
//     called.
//
// @pre `obj` is not a null object.
// @post the returned value is not a null object (it evaluates to true).
::py::object getPartialSelf(const ::py::function& partialFunc)
{
  QI_ASSERT_TRUE(partialFunc);

  GILAcquire lock;

  constexpr const char* argsAttr = "args";
  constexpr const char* funcAttr = "func";
  if (!::py::hasattr(partialFunc, argsAttr) || !::py::hasattr(partialFunc, funcAttr))
    // It's not a partial func, return immediately.
    return ::py::none();

  try
  {
    auto func = partialFunc.attr(funcAttr).cast<::py::function>();
    QI_ASSERT_TRUE(func);

    // The function might be a (bound) method.
    const auto self = getMethodSelf(func);
    if (!self.is_none())
      return self;

    // The function might be a `partial`.
    const auto subpartialSelf = getPartialSelf(func);
    if (!subpartialSelf.is_none())
      return subpartialSelf;

    // The function might be an unbound method.
    //
    // Unlike in Python 2, there is no known way to differentiate between an
    // unbound method and a free function in Python 3. So to maintain
    // retrocompatibility with older versions of libqi-python, we still decide
    // to assume that the first argument is an instance (`self`), even though
    // the function might be a free function, and not an unbound function. Then
    // we check the function is a reference to one of the function attribute of
    // that first argument.
    //
    // We can also check if the function is a not static method (which won't
    // have a self parameter).
    const bool isStaticMethod = ::py::isinstance<::py::staticmethod>(func);
    if (!isStaticMethod)
    {
      ::py::tuple args = partialFunc.attr(argsAttr);
      if (args.empty())
        return ::py::none();
      const ::py::object selfCandidate = args[0];
      if (isUnboundMethod(func, selfCandidate))
        return selfCandidate;
    }

    // Fallback to just returning the function.
    return std::move(func);
  }
  catch (const std::exception& ex)
  {
    qiLogVerbose()
      << "An exception occurred when extracting a partial function: "
      << ex.what();
    return ::py::none();
  }
}

// Returns the instance of the object on which the function is called (the
// `self` parameter).
//
// This function tries to detect partial application of functions and deduce
// the instance from the arguments, if passed.
//
// @pre `func` is not a null object.
// @post the returned value is not a null object (it evaluates to true).
::py::object getSelf(const ::py::function& func)
{
  QI_ASSERT_TRUE(func);

  GILAcquire lock;
  const auto self = getMethodSelf(func);
  if (!self.is_none())
    return self;
  return getPartialSelf(func);
}

} // namespace

StrandPtr strandOfFunction(const ::py::function& func)
{
  GILAcquire lock;
  return strandOf(getSelf(func));
}

StrandPtr strandOf(const ::py::object& obj)
{
  QI_ASSERT_TRUE(obj);

  GILAcquire lock;

  if (obj.is_none())
    return {};

  if (isMultithreaded(obj))
    return {};

  auto strandObj = ::py::getattr(obj, objectAttributeStrandName, ::py::none());
  if (strandObj.is_none())
  {
    try
    {
      strandObj = ::py::cast(StrandPtr(new Strand, DeleteOutsideGIL()));
      ::py::setattr(obj, objectAttributeStrandName, strandObj);
    }
    catch (const ::py::error_already_set& ex)
    {
      // If setting the attribute fails with an AttributeError, it may mean that
      // we cannot set attributes on this object, therefore it cannot have an
      // associated strand.
      if (ex.matches(PyExc_AttributeError)) return {};
      throw;
    }
  }

  if (strandObj.is_none() || !::py::isinstance<Strand>(strandObj))
    return {};

  return strandObj.cast<StrandPtr>();
}

bool isMultithreaded(const ::py::object& obj)
{
  QI_ASSERT_TRUE(obj);

  GILAcquire lock;
  const auto pyqisig = ::py::getattr(obj, objectAttributeThreadingName, ::py::none());
  if (pyqisig.is_none())
    return false;
  return pyqisig.cast<std::string>() == objectAttributeThreadingValueMulti;
}

void exportStrand(::py::module& m)
{
  using namespace ::py;

  GILAcquire lock;

  class_<Strand, StrandPtr>(m, "Strand")
    .def(init([] {
      return StrandPtr(new Strand, DeleteOutsideGIL());
    }));
}

} // namespace py
} // namespace qi
