/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYTYPES_HPP
#define QIPYTHON_PYTYPES_HPP

#include <qipython/common.hpp>
#include <qi/anyvalue.hpp>

namespace qi
{
namespace py
{

pybind11::object unwrapValue(AnyReference val);

/// Introspects a Python object to create a `qi::AnyReference` around its value
/// with the corresponding type.
///
/// With 'exactly a T' meaning that the object's type is the type T and not one
/// of its subclasses, (note that bool cannot be used as a base type), the type
/// kind of the qi type of the result depends whether the object is:
///   - `None`: TypeKind_Void.
///   - exactly an integer: TypeKind_Int (the size returned by the type
///     interface is `sizeof(long long)`).
///   - exactly a float: TypeKind_Float (the size returned by the type
///     interface is `sizeof(double)`).
///   - exactly a bool: TypeKind_Int (the size returned by the type interface
///     is 0).
///   - exactly a unicode string: TypeKind_Str.
///   - exactly a byte array or bytes: TypeKind_Str.
///   - exactly a tuple: TypeKind_Tuple.
///   - exactly a set (or frozenset): TypeKind_Tuple.
///   - exactly a list: TypeKind_List.
///   - exactly a dict: TypeKind_Map.
///
/// The function will throw an exception if the object is:
///   - an ellipsis.
///   - exactly a complex.
///   - a memoryview.
///   - a slice.
///   - a module.
///
/// If the object type is none of the supported or unsupported types, it is
/// introspected and a `qi::py::Object` is constructed out of it by executing
/// `qi::py::toObject`.
///
/// @pre `obj`
AnyReference unwrapAsRef(pybind11::object& obj);

void registerTypes();

}
}

#endif // QIPYTHON_PYTYPES_HPP
