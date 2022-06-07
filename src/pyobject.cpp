/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <qipython/pyobject.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pysignal.hpp>
#include <qipython/pyproperty.hpp>
#include <qipython/pystrand.hpp>
#include <qi/type/dynamicobjectbuilder.hpp>
#include <qi/jsoncodec.hpp>
#include <qi/strand.hpp>
#include <qi/session.hpp>
#include <pybind11/operators.h>

qiLogCategory("qi.python.object");

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

constexpr static const auto qiObjectUidAttributeName = "__qi_objectuid__";
constexpr static const auto qiNameAttributeName = "__qi_name__";
constexpr static const auto qiSignatureAttributeName = "__qi_signature__";
constexpr static const auto qiSignatureAttributeDoNotBindValue = "DONOTBIND";
constexpr static const auto qiReturnSignatureAttributeName = "__qi_return_signature__";
constexpr static const auto asyncArgName = "_async";
constexpr static const auto overloadArgName = "_overload";

// Calls the function of a qi Object, with a list of Python arguments.
::py::object call(const Object& obj, std::string funcName,
                  ::py::args args, ::py::kwargs kwargs)
{
  GILAcquire lock;

  if (auto optOverload = extractKeywordArg<std::string>(kwargs, overloadArgName))
    funcName = *optOverload;

  auto async = false;
  if (auto optAsync = extractKeywordArg<bool>(kwargs, asyncArgName))
    async = *optAsync;

  auto argsValue = AnyValue::from(args);

  Promise prom;
  {
    GILRelease _unlock;
    auto metaCallFut = obj.metaCall(funcName, argsValue.asTupleValuePtr(),
                                    async ? MetaCallType_Queued : MetaCallType_Direct);

    // `adaptFutureUnwrap` supports `AnyReference` containing a `Future`, so
    // `Future<AnyReference>` will be unwrapped if the `AnyReference` is itself
    // a `Future`.
    adaptFutureUnwrap(metaCallFut, prom);
  }

  return resultObject(prom.future(), async);
}

std::string docString(const MetaMethod& method)
{
  std::ostringstream oss;
  oss << method.toString() << " -> " << method.returnSignature() << "\n"
      << method.description();
  return oss.str();
}

void populateMethods(::py::object pyobj, const Object& obj)
{
  GILAcquire lock;

  const auto& metaObj = obj.metaObject();
  const auto& methodMap = metaObj.methodMap();
  for (const auto& methodSlot : methodMap)
  {
    const auto& method = methodSlot.second;
    const auto& methodName = method.name();

    // Drop special members.
    if (method.uid() < qiObjectSpecialMemberMaxUid)
      continue;

    namespace sph = std::placeholders;

    const auto doc = docString(method);
    std::function<::py::object(::py::args, ::py::kwargs)> callMethod =
      std::bind(&call, obj, method.name(), sph::_1, sph::_2);
    ::py::setattr(pyobj, methodName.c_str(),
                  ::py::cpp_function(std::move(callMethod),
                                     ::py::is_method(pyobj.get_type()),
                                     ::py::doc(doc.c_str())));
  }
}

void populateSignals(::py::object pyobj, const Object& obj)
{
  const auto& metaObj = obj.metaObject();
  const auto& signalMap = metaObj.signalMap();
  for (const auto& signalSlot : signalMap)
  {
    const auto& signal = signalSlot.second;
    const auto& signalName = signal.name();

    // Do not add a signal that is also a property, it must be added as a
    // property.
    if (metaObj.propertyId(signalName) != -1)
      continue;

    // Drop special members.
    if (signal.uid() < qiObjectSpecialMemberMaxUid)
      continue;

    ::py::setattr(pyobj, signalName.c_str(),
                  castToPyObject(detail::ProxySignal{ obj, signal.uid() }));
  }
}

void populateProperties(::py::object pyobj, const Object& obj)
{
  const auto propMap = obj.metaObject().propertyMap();
  for (const auto& propSlot : propMap)
  {
    const auto& prop = propSlot.second;
    const auto& propName = prop.name();

    // Drop special members.
    if (prop.uid() < qiObjectSpecialMemberMaxUid)
      continue;

    ::py::setattr(pyobj, propName.c_str(),
                  castToPyObject(detail::ProxyProperty{ obj, prop.uid() }));
  }
}

using GenericFunction = std::function<::py::object(::py::args)>;

// Returns an owning AnyReference (it must be explicitly destroyed).
//
// @pre `cargs.size() > 0`, arguments must at least contain a `DynamicObject`
//      on which the function is to be called.
AnyReference callPythonMethod(const AnyReferenceVector& cargs,
                              const GILGuardedObject& method)
{
  auto it = cargs.begin();
  const auto cargsEnd = cargs.end();

  // Drop the first arg which is a `DynamicObject*`.
  QI_ASSERT_TRUE(it != cargsEnd);
  ++it;

  GILAcquire lock;
  ::py::tuple args(std::distance(it, cargsEnd));

  ::py::size_t i = 0;
  while (it != cargsEnd)
  {
    QI_ASSERT_TRUE(i < args.size());
    args[i++] = castToPyObject(*it++);
  }

  // Convert Python future object into a C++ Future, to allow libqi to unwrap
  // it.
  const ::py::object ret = (*method)(*args);
  if (::py::isinstance<Future>(ret))
    return AnyValue::from(ret.cast<Future>()).release();
  return AnyReference::from(ret).content().clone();
}

// Gets the default signature for a method.
//
// If the function takes variadic arguments (vargs), returns the signature of a
// pure dynamic element, which indicates a generic function that takes anything.
//
// Otherwise, returns the signature of a function taking n Python objects, with n
// the number of positional parameters the function accepts.
std::string methodDefaultParametersSignature(const ::py::function& method)
{
  GILAcquire lock;

  // Returns a Signature object (see
  // https://docs.python.org/3/library/inspect.html#inspect.Signature).
  const auto inspect = ::py::module::import("inspect");
  const ::py::object pySignature = inspect.attr("signature")(method);
  const ::py::object paramType = inspect.attr("Parameter");

  const auto parameterKind = [](const ::py::handle& obj) -> ::py::object {
    return obj.attr("kind");
  };

  const ::py::object varPositional = paramType.attr("VAR_POSITIONAL");
  const auto isVarPositional = [&](const std::pair<::py::handle, ::py::handle>& param) {
    return parameterKind(param.second).equal(varPositional);
  };

  // If there is any vargs parameter, return `m`.
  const ::py::dict parameters = pySignature.attr("parameters");
  if (std::any_of(parameters.begin(), parameters.end(), isVarPositional))
    return Signature::fromType(Signature::Type_Dynamic).toString();

  const auto positionalOnly = paramType.attr("POSITIONAL_ONLY");
  ::py::object positionalOrKeyword = paramType.attr("POSITIONAL_OR_KEYWORD");
  const auto isPositional = [&](const std::pair<::py::handle, ::py::handle>& param) {
    const auto kind = parameterKind(param.second);
    return kind.equal(positionalOnly) || kind.equal(positionalOrKeyword);
  };

  const auto argsCount =
    std::count_if(parameters.begin(), parameters.end(), isPositional);
  return makeTupleSignature(
           std::vector<TypeInterface*>(argsCount, typeOf<::py::object>()))
    .toString();
}

boost::optional<unsigned int> registerMethod(DynamicObjectBuilder& gob,
                                             const std::string& name,
                                             const ::py::function& method,
                                             std::string parametersSignature)
{
  GILAcquire lock;

  if (boost::starts_with(name, "__"))
  {
    qiLogVerbose() << "Registration of method " << name << " is ignored as it is private.";
    return {};
  }

  MetaMethodBuilder mmb;
  mmb.setName(name);

  ::py::object desc = method.doc();
  if (desc)
    mmb.setDescription(::py::str(desc));

  if (parametersSignature.empty())
    parametersSignature = methodDefaultParametersSignature(method);
  mmb.setParametersSignature(parametersSignature);

  std::string returnSignature;
  const auto pyqiretsig = ::py::getattr(method, qiReturnSignatureAttributeName, ::py::none());
  if (!pyqiretsig.is_none())
    returnSignature = ::py::str(pyqiretsig);

  if (returnSignature.empty())
    returnSignature = Signature::Type_Dynamic;
  mmb.setReturnSignature(returnSignature);

  qiLogVerbose() << "Registration of method " << name << " with signature "
                 << parametersSignature << " -> " << returnSignature << ".";

  return gob.xAdvertiseMethod(mmb, AnyFunction::fromDynamicFunction(
                                     boost::bind(callPythonMethod, _1,
                                                 GILGuardedObject(method))));
}

} // namespace

namespace detail
{

boost::optional<ObjectUid> readObjectUid(const ::py::object& obj)
{
  GILAcquire lock;
  const auto qiObjectUidObj = ::py::getattr(obj, qiObjectUidAttributeName, ::py::none());
  if (qiObjectUidObj.is_none())
    return {};
  const auto qiObjectUid = qiObjectUidObj.cast<std::string>();
  return deserializeObjectUid(qiObjectUid);
}

void writeObjectUid(const pybind11::object& obj, const ObjectUid& uid)
{
  GILAcquire lock;
  const ::py::bytes uidData = serializeObjectUid<std::string>(uid);
  ::py::setattr(obj, qiObjectUidAttributeName, uidData);
}

} // namespace detail


::py::object toPyObject(Object obj)
{
  auto result = castToPyObject(obj);

  if (!obj.isValid())
    return result;

  const auto objType = obj.asGenericObject()->type;
  if (objType == typeOf<Future>())
    return castToPyObject(qi::Object<Future>(obj).asT());
  if (objType == typeOf<FutureSync<AnyValue>>())
    return castToPyObject(qi::Object<FutureSync<AnyValue>>(obj)->async());
  if (objType == typeOf<Promise>())
    return castToPyObject(qi::Object<Promise>(obj).asT());

  populateMethods(result, obj);
  populateSignals(result, obj);
  populateProperties(result, obj);
  return result;
}

Object toObject(const ::py::object& obj)
{
  GILAcquire lock;

  // Check the few known Object types that a Python object can be.
  if (::py::isinstance<Object>(obj))
    return obj.cast<Object>();

  if (::py::isinstance<Future>(obj))
  {
    auto fut = obj.cast<Future>();
    return Object(boost::make_shared<Future>(fut));
  }

  if (::py::isinstance<Promise>(obj))
  {
    auto prom = obj.cast<Promise>();
    return Object(boost::make_shared<Promise>(prom));
  }

  DynamicObjectBuilder gob;
  gob.setThreadingModel(isMultithreaded(obj) ? ObjectThreadingModel_MultiThread
                                             : ObjectThreadingModel_SingleThread);

  const auto attrKeys = ::py::reinterpret_steal<::py::list>(PyObject_Dir(obj.ptr()));
  for (const ::py::handle& pyAttrKey : attrKeys)
  {
    QI_ASSERT_TRUE(pyAttrKey);
    QI_ASSERT_FALSE(pyAttrKey.is_none());
    QI_ASSERT_TRUE(::py::isinstance<::py::str>(pyAttrKey));

    const auto attrKey = pyAttrKey.cast<std::string>();
    auto memberName = attrKey;

    const ::py::object attr = obj.attr(pyAttrKey);
    if (attr.is_none())
    {
      qiLogVerbose() << "The object attribute '" << attrKey
                     << "' has value 'None', and will therefore be ignored.";
      continue;
    }

    std::string signature;
    const auto pyqisig = ::py::getattr(attr, qiSignatureAttributeName, ::py::none());
    if (!pyqisig.is_none())
      signature = ::py::str(pyqisig);

    if (signature == qiSignatureAttributeDoNotBindValue)
      continue;

    const auto pyqiname = ::py::getattr(attr, qiNameAttributeName, ::py::none());
    if (!pyqiname.is_none())
      memberName = ::py::str(pyqiname);

    if (::py::isinstance<Signal>(attr))
    {
      auto sig = ::py::cast<Signal*>(attr);
      gob.advertiseSignal(memberName, sig);
      continue;
    }

    if (::py::isinstance<Property>(attr))
    {
      auto prop = ::py::cast<Property*>(attr);
      gob.advertiseProperty(memberName, prop);
      continue;
    }

    if (::py::isinstance<::py::function>(attr))
    {
      registerMethod(gob, memberName, attr, signature);
      continue;
    }
  }

  // If we find an ObjectUid in the python object, reuse it.
  auto maybeObjectUid = detail::readObjectUid(obj);
  if (maybeObjectUid)
  {
    // This object already has an ObjectUid: reuse it.
    gob.setOptionalUid(maybeObjectUid);
  }

  // This is a useless callback, needed to keep a reference on the python object.
  // When the GenericObject is destroyed, the reference is released.
  GILGuardedObject guardedObj(obj);
  Object anyobj = gob.object([guardedObj](GenericObject*) mutable {
      GILAcquire lock;
      *guardedObj = {};
    });

  // If there was no ObjectUid stored in the python object, store a new one.
  if (!maybeObjectUid)
    // First time we make an AnyObject for this python object: store the new ObjectUid in it.
    detail::writeObjectUid(obj, anyobj.uid());

  // At this point, both the AnyObject and the related python object have the same ObjectUid value.
  QI_ASSERT_TRUE(anyobj.uid() == detail::readObjectUid(obj));

  if (const auto strand = strandOf(obj))
    anyobj.forceExecutionContext(strand);

  return anyobj;
}

void exportObject(::py::module& m)
{
  using namespace ::py;
  using namespace ::py::literals;

  GILAcquire lock;

  class_<Object>(m, "Object", dynamic_attr())
    .def(self == self, call_guard<GILRelease>())
    .def(self != self, call_guard<GILRelease>())
    .def(self < self, call_guard<GILRelease>())
    .def(self <= self, call_guard<GILRelease>())
    .def(self > self, call_guard<GILRelease>())
    .def(self >= self, call_guard<GILRelease>())
    .def("__bool__", &Object::isValid, call_guard<GILRelease>())
    .def("isValid", &Object::isValid, call_guard<GILRelease>())
    .def("call",
         [](Object& obj, const std::string& funcName, ::py::args args,
            ::py::kwargs kwargs) {
           return call(obj, funcName, std::move(args), std::move(kwargs));
         },
         "funcName"_a)
    .def("async",
         [](Object& obj, const std::string& funcName, ::py::args args,
            ::py::kwargs kwargs) {
           kwargs[asyncArgName] = true;
           return call(obj, funcName, std::move(args), std::move(kwargs));
         },
         "funcName"_a)
    .def("metaObject",
         [](const Object& obj) { return AnyReference::from(obj.metaObject()); },
         call_guard<GILRelease>());
  // TODO: .def("post")
  // TODO: .def("setProperty")
  // TODO: .def("property")
}

} // namespace py
} // namespace qi
