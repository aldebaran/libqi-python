/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_PYPROPERTY_HPP
#define QIPYTHON_PYPROPERTY_HPP

#include <qipython/common.hpp>
#include <qi/property.hpp>
#include <qi/anyobject.hpp>
#include <memory>

namespace qi
{
namespace py
{

using Property = qi::GenericProperty;

namespace detail
{
  struct ProxyProperty
  {
    AnyWeakObject object;
    unsigned int propertyId;
  };
}

/// Checks if an object is an instance of a property.
///
/// Unfortunately, as property types are a bit messy in libqi, we don't have
/// an easy way to test that an object is an instance of a Property. This
/// function tries to hide the details of the underlying types.
bool isProperty(const pybind11::object& obj);

void exportProperty(pybind11::module& module);

}
}

#endif // QIPYTHON_PYPROPERTY_HPP
