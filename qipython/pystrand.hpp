/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYSTRAND_HPP
#define QIPYTHON_PYSTRAND_HPP

#include <qipython/common.hpp>
#include <qi/strand.hpp>
#include <boost/shared_ptr.hpp>

namespace qi
{
namespace py
{

using StrandPtr = boost::shared_ptr<qi::Strand>;

/// Returns the strand for the given function.
///
/// If the function is bound to an instance, returns the strand of that object.
///
/// @pre `func` is not a null object.
StrandPtr strandOfFunction(const pybind11::function& func);

/// Returns the strand associated to an object.
///
/// If the object is `None`, returns nullptr.
/// If the object is tagged as multithreaded (see the `isMultithreaded`
/// function), returns nullptr.
///
/// If the object already has a strand associated to it, returns it. Otherwise
/// the function tries to create a new one and associate it to the object by
/// setting it as an attribute of the object. If new attributes cannot be set on
/// the object, returns nullptr.
///
/// @pre `object` is not a null object.
StrandPtr strandOf(const pybind11::object& obj);

/// Returns true if the object was declared as multithreaded, false otherwise.
///
/// An object is multithreaded only if it is declared as such (by having an attribute
/// `__qi_threading__` set to `multi`. In any other case it is, either explicitly or implicitly,
/// singlethreaded.
///
/// @pre `obj` is not a null object.
bool isMultithreaded(const pybind11::object& obj);

void exportStrand(pybind11::module& module);

} // namespace py
} // namespace qi

#endif // QIPYTHON_PYSTRAND_HPP
