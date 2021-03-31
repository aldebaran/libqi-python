/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_COMMON_HPP
#define QIPYTHON_COMMON_HPP

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <qipython/pytypes.hpp>
#include <ka/errorhandling.hpp>
#include <ka/functional.hpp>
#include <future>
#include <thread>

namespace qi
{
namespace py
{

/// Returns the instance of the bound method if it exists, otherwise returns
/// a `None` object.
///
/// @pre `obj` is not a null object.
/// @pre The GIL is locked.
/// @post the returned value is not a null object (it evaluates to true).
inline pybind11::object getMethodSelf(const pybind11::function& func)
{
  QI_ASSERT_TRUE(func);
  if (PyMethod_Check(func.ptr()))
    return pybind11::reinterpret_borrow<pybind11::object>(
      PyMethod_GET_SELF(func.ptr()));
  return pybind11::none();
}

/// Casts a value into a Python object, throwing an exception if the conversion
/// fails.
///
/// @pre The GIL is locked.
/// @post the returned value is not a null object (it evaluates to true).
// See https://github.com/pybind/pybind11/issues/2336
// Casting does not throw if the type is unregistered, so we throw explicitly in
// case of an error.
// TODO: Remove this function and its uses when the issue is fixed.
template<typename T, typename... Extra,
  ka::EnableIf<!std::is_base_of<pybind11::handle, ka::RemoveCvRef<T>>::value,
               int> = 0>
pybind11::object castToPyObject(T&& t, Extra&&... extra)
{
  auto obj = pybind11::cast(std::forward<T>(t), std::forward<Extra>(extra)...);
  if (PyErr_Occurred())
    throw pybind11::error_already_set();
  QI_ASSERT_TRUE(obj);
  return obj;
}

/// Extracts a keyword argument out of a dictionary of keywords arguments.
///
/// If the argument is not in the dictionary, does nothing and returns an empty
/// optional.
///
/// If the argument is is the dictionary, it is removed from it and its value is
/// then cast to a value of type `T` and an optional of this value is returned.
///
/// If the argument has value `None`, an empty optional is returned unless
/// `allowNone` is set to true, in which case the `None` value will be cast to
/// a value of type `T`.
///
/// @pre The GIL is locked.
/// @warning If the value of the argument cannot be cast to type `T`, an
///          exception is thrown but the argument is still removed from the
///          dictionary.
template <typename T>
boost::optional<T> extractKeywordArg(pybind11::dict kwargs,
                                     const char* argName,
                                     bool allowNone = false)
{
  if (!kwargs.contains(argName))
    return {};

  const pybind11::object pyArg = kwargs[argName];
  PyDict_DelItemString(kwargs.ptr(), argName);

  if (pyArg.is_none() && !allowNone)
    return {};

  return pyArg.cast<T>();
}

/// A deleter that deletes the pointer outside of the GIL.
///
/// Useful for types that might deadlock on destruction if they keep the GIL
/// locked.
struct DeleteOutsideGIL
{
  template<typename T>
  void operator()(T* ptr) const
  {
    pybind11::gil_scoped_release unlock;
    delete ptr;
  }
};

/// Delay the destruction of an object to a thread.
struct DeleteInOtherThread
{
  template<typename T>
  void operator()(T* ptr) const
  {
    pybind11::gil_scoped_release unlock;
    auto fut = std::async(std::launch::async, [](std::unique_ptr<T>) {},
                          std::unique_ptr<T>(ptr));
    QI_IGNORE_UNUSED(fut);
  }
};

} // namespace py
} // namespace qi

namespace pybind11
{
namespace detail
{
  template <>
  struct type_caster<qi::AnyReference>
  {
  public:
    PYBIND11_TYPE_CASTER(qi::AnyReference, _("qi_python.AnyReference"));

    // Python -> C++
    bool load(handle src, bool /* enable implicit conversions */)
    {
      QI_ASSERT_TRUE(src);
      value = ka::invoke_catch(ka::compose(
                                 [&](const std::string&) {
                                   currentObject = {};
                                   return qi::AnyReference{};
                                 },
                                 ka::exception_message_t()),
                               [&] {
                                 currentObject =
                                   reinterpret_borrow<object>(src);
                                 return qi::py::unwrapAsRef(&currentObject);
                               });
      return value.isValid();
    }

    // C++ -> Python
    static handle cast(qi::AnyReference src,
                       return_value_policy /* policy */,
                       handle /* parent */)
    {
      return qi::py::unwrapValue(src).release();
    }

    object currentObject;
  };

  template <>
  struct type_caster<qi::AnyValue>
  {
  public:
    PYBIND11_TYPE_CASTER(qi::AnyValue, _("qi_python.AnyValue"));

    using RefCaster = type_caster<qi::AnyReference>;

    // Python -> C++
    bool load(handle src, bool enableImplicitConv)
    {
      QI_ASSERT_TRUE(src);
      RefCaster refCaster;
      refCaster.load(src, enableImplicitConv);
      value.reset(std::move(refCaster));
      return value.isValid();
    }

    // C++ -> Python
    static handle cast(qi::AnyValue src,
                       return_value_policy policy,
                       handle parent)
    {
      return RefCaster::cast(src.asReference(), policy, parent);
    }
  };

  template <typename T>
  struct type_caster<boost::optional<T>> : optional_caster<boost::optional<T>>
  {};

} // namespace detail
} // namespace pybind11

PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

#endif // QIPYTHON_COMMON_HPP
