/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYOBJECT_HPP
#define QIPYTHON_PYOBJECT_HPP

#include <qipython/common.hpp>
#include <qi/anyobject.hpp>

namespace qi
{
namespace py
{

namespace detail
{

boost::optional<ObjectUid> readObjectUid(const pybind11::object& obj);

/// @invariant `(writeObjectUid(obj, uid), *readObjectUid(obj)) == uid`
void writeObjectUid(const pybind11::object& obj, const ObjectUid& uid);

} // namespace detail

using Object = qi::AnyObject;

/// @post the returned value is not a null object (it evaluates to true).
pybind11::object toPyObject(Object obj);

/// Converts a Python object into a qi Object.
///
/// If the Python object is already a wrapper around a C++ type that is known
/// to be registered as an Object type in the qi type system, it is converted
/// into that type and then into a qi `AnyObject`.
///
/// Otherwise, a dynamic qi Object is created by introspection of the member
/// attributes of the Python object.
///
/// Member attributes may have an explicitly associated signature and/or return
/// signature set as their own attributes. The name can also be set as their
/// attribute in which case it overrides the member attribute name.
///
/// If the signature has some special value that reflects the intention of
/// the user to not bind this member, the member is ignored.
///
/// The member might be a callable (method or function). It is then exposed
/// as a method of the `qi::Object` using the signature and return signatures
/// either deduced from the function object or from its attributes.
/// Functions that have a name starting with two underscores are considered
/// private and therefore ignored.
///
/// The member might also be a Property or a Signal, in which case it is
/// exposed as such in the `qi::Object`, but its signature is always
/// dynamic (its attributes are ignored).
///
/// Furthermore, the function will ensure that any object UID existing in the
/// Python object is set in the resulting dynamic `qi::Object`.
Object toObject(const pybind11::object& obj);

void exportObject(pybind11::module& module);

}
}

#endif  // QIPYTHON_PYOBJECT_HPP
