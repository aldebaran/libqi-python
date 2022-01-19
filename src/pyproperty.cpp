/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyproperty.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pysignal.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pystrand.hpp>
#include <pybind11/pybind11.h>
#include <qi/anyobject.hpp>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

constexpr static const auto asyncArgName = "_async";

pybind11::object propertyConnect(Property& prop,
                                 const pybind11::function& pyCallback,
                                 bool async)
{
  GILAcquire lock;
  return detail::signalConnect(prop, pyCallback, async);
}


::py::object proxyPropertyConnect(detail::ProxyProperty& prop,
                                  const ::py::function& callback,
                                  bool async)
{
  GILAcquire lock;
  return detail::proxySignalConnect(prop.object, prop.propertyId, callback, async);
}

} // namespace

detail::ProxyProperty::~ProxyProperty()
{
  // The destructor can lock when waiting for callbacks to end.
  GILRelease unlock;
  object.reset();
}

bool isProperty(const pybind11::object& obj)
{
  GILAcquire lock;
  return ::py::isinstance<Property>(obj) ||
         ::py::isinstance<detail::ProxyProperty>(obj);
}

void exportProperty(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  using PropertyPtr = std::unique_ptr<Property, DeleteOutsideGIL>;
  class_<Property, PropertyPtr>(m, "Property")

    .def(init([] { return new Property(TypeInterface::fromSignature("m")); }),
         call_guard<GILRelease>())

    .def(init([](const std::string& sig) {
           return PropertyPtr(new Property(TypeInterface::fromSignature(sig)),
                              DeleteOutsideGIL());
         }),
         "signature"_a, call_guard<GILRelease>())

    .def("value",
         [](const Property& prop, bool async) {
           const auto fut = prop.value().async();
           GILAcquire lock;
           return resultObject(fut, async);
         },
         arg(asyncArgName) = false, call_guard<GILRelease>(),
         doc("Return the value stored inside the property."))

    .def("setValue",
         [](Property& prop, AnyValue value, bool async) {
           const auto fut = toFuture(prop.setValue(std::move(value)).async());
           GILAcquire lock;
           return resultObject(fut, async);
         },
         call_guard<GILRelease>(), "value"_a, arg(asyncArgName) = false,
         doc("Set the value of the property."))

    .def("addCallback", &propertyConnect, "cb"_a, arg(asyncArgName) = false,
         doc("Add an event subscriber to the property.\n"
             ":param cb: the callback to call when the property changes\n"
             ":returns: the id of the property subscriber"))

    .def("connect", &propertyConnect, "cb"_a, arg(asyncArgName) = false,
         doc("Add an event subscriber to the property.\n"
             ":param cb: the callback to call when the property changes\n"
             ":returns: the id of the property subscriber"))

    .def("disconnect",
         [](Property& prop, SignalLink id, bool async) {
           return detail::signalDisconnect(prop, id, async);
         },
         call_guard<GILRelease>(), "id"_a, arg(asyncArgName) = false,
         doc("Disconnect the callback associated to id.\n\n"
             ":param id: the connection id returned by :method:connect or "
             ":method:addCallback\n"
             ":returns: true on success"))

    .def("disconnectAll",
         [](Property& prop, bool async) {
           return detail::signalDisconnectAll(prop, async);
         },
         call_guard<GILRelease>(), arg(asyncArgName) = false,
         doc("Disconnect all subscribers associated to the property.\n\n"
             "This function should be used with caution, as it may also remove "
             "subscribers that were added by other callers.\n\n"
             ":returns: true on success\n"));

  class_<detail::ProxyProperty>(m, "_ProxyProperty")
    .def("value",
         [](const detail::ProxyProperty& prop, bool async) {
           const auto fut = prop.object.property(prop.propertyId).async();
           GILAcquire lock;
           return resultObject(fut, async);
         },
         call_guard<GILRelease>(), arg(asyncArgName) = false)
    .def("setValue",
         [](detail::ProxyProperty& prop, object pyValue, bool async) {
           AnyValue value(unwrapAsRef(pyValue));
           GILRelease unlock;
           const auto fut =
             toFuture(prop.object.setProperty(prop.propertyId, std::move(value))
                        .async());
           GILAcquire lock;
           return resultObject(fut, async);
         },
         "value"_a, arg(asyncArgName) = false)
    .def("addCallback", &proxyPropertyConnect, "cb"_a,
         arg(asyncArgName) = false)
    .def("connect", &proxyPropertyConnect, "cb"_a,
         arg(asyncArgName) = false)
    .def("disconnect",
         [](detail::ProxyProperty& prop, SignalLink id, bool async) {
           return detail::proxySignalDisconnect(prop.object, id, async);
         },
         "id"_a, arg(asyncArgName) = false);
}

} // namespace py
} // namespace qi
