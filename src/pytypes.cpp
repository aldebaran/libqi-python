/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#include <utility>
#include <tuple>
#include <iterator>
#include <qi/type/typedispatcher.hpp>
#include <qi/type/typeinterface.hpp>
#include <qi/numeric.hpp>
#include <qi/assert.hpp>
#include <qipython/common.hpp>
#include <qipython/pyguard.hpp>
#include <qipython/pytypes.hpp>
#include <qipython/pyfuture.hpp>
#include <qipython/pyobject.hpp>
#include <pybind11/pybind11.h>
#include <boost/thread/synchronized_value.hpp>

namespace py = pybind11;

namespace qi
{
namespace py
{

namespace
{

struct ObjectDecRef
{
  ::py::handle obj;
  void operator()() const
  {
    GILAcquire lock;
    obj.dec_ref();
  }
};

template<typename T>
boost::optional<::py::object> tryToCastObjectTo(ObjectTypeInterface* type,
                                                void* ptr)
{
  if (type != typeOf<T>())
    return {};
  return castToPyObject(reinterpret_cast<T*>(ptr), ::py::return_value_policy::copy);
}

struct ValueToPyObject
{
  // @pre: The GIL is locked.
  ValueToPyObject(::py::object& result)
    : result(result)
  {
  }

  void visitUnknown(AnyReference value)
  {
    GILAcquire lock;
    // Encapsulate the value in Capsule.
    result = ::py::capsule(value.rawValue());
  }

  void visitVoid()
  {
    GILAcquire lock;
    result = ::py::none();
  }

  void visitInt(int64_t value, bool isSigned, int byteSize)
  {
    GILAcquire lock;
    // byteSize is 0 when the value is a boolean.
    if (byteSize == 0)
      result = ::py::bool_(static_cast<bool>(value));
    else
      result = isSigned ? ::py::int_(value)
                        : ::py::int_(static_cast<std::uint64_t>(value));
  }

  void visitFloat(double value, int /*byteSize*/)
  {
    GILAcquire lock;
    result = ::py::float_(value);
  }

  void visitString(char* data, size_t len)
  {
    GILAcquire lock;

    if (!data)
    {
      result = ::py::str("");
      return;
    }

    // Python strings are Unicode, and trying to construct one with a characters
    // sequence that is not valid UTF-8 data results in an exception in
    // pybind11. So if we can't create a Python `string` due to an exception, we
    // try to create a Python `bytes` instead.
    const auto toUnicode = [&] {
      const auto obj = ::py::reinterpret_steal<::py::str>(
        PyUnicode_FromStringAndSize(data, static_cast<ssize_t>(len)));
      // When it fails, the conversion may set a Python error. We need to handle
      // it by throwing `error_already_set`.
      if (!obj)
      {
        if (PyErr_Occurred())
          throw ::py::error_already_set();
        else
          throw std::runtime_error("failed to construct a Python string from data");
      }
      return obj;
    };
    const auto toBytes = [&] { return ::py::bytes(data, len); };

    using namespace ka::functional_ops;
    const auto thenToBytes = ka::constant_function() | toBytes;
    result = ka::invoke_catch(thenToBytes, toUnicode);
  }

  void visitList(AnyIterator it, AnyIterator end)
  {
    GILAcquire lock;
    ::py::list l;
    for (; it != end; ++it)
      l.append(unwrapValue(*it));
    result = l;
  }

  void visitVarArgs(AnyIterator it, AnyIterator end)
  {
    visitList(it, end);
  }

  void visitMap(AnyIterator it, AnyIterator end)
  {
    GILAcquire lock;
    ::py::dict d;
    for (; it != end; ++it)
      d[unwrapValue((*it)[0])] = (*it)[1];
    result = d;
  }

  void visitObject(GenericObject go)
  {
    if (!go.isValid())
      throw std::runtime_error("cannot convert an invalid generic object");

    const auto type = go.type;
    const auto ptr = type->ptrFromStorage(&go.value);

    GILAcquire lock;
    if (auto obj = tryToCastObjectTo<Future>(type, ptr))
    {
      result = *obj;
      return;
    }
    if (auto obj = tryToCastObjectTo<Promise>(type, ptr))
    {
      result = *obj;
      return;
    }

    throw std::runtime_error("cannot convert a generic object without refcount to a Python object");
  }

  void visitAnyObject(AnyObject& obj)
  {
    GILAcquire lock;
    result = py::toPyObject(obj);
  }

  void visitPointer(AnyReference)
  {
    throw std::runtime_error("cannot convert a pointer to a Python object");
  }

  void visitTuple(const std::string& /*name*/,
                  const std::vector<AnyReference>& tuple,
                  const std::vector<std::string>& annotations)
  {
    const auto len = tuple.size();

    GILAcquire lock;
    if (annotations.empty())
    {
      // Unnamed tuple
      ::py::tuple t(len);
      for (std::size_t i = 0; i < len; ++i)
        t[i] = tuple[i];
      result = t;
    }
    else
    {
      QI_ASSERT_TRUE(annotations.size() <= tuple.size());
      ::py::dict d;
      for (std::size_t i = 0; i < annotations.size(); ++i)
        d[annotations.at(i).c_str()] = tuple[i];
      result = d;
    }
  }

  void visitDynamic(AnyReference pointee)
  {
    GILAcquire lock;
    result = unwrapValue(pointee);
  }

  void visitRaw(AnyReference value)
  {
    /* TODO: zerocopy, sub-buffers... */
    const auto dataWithSize = value.asRaw();

    GILAcquire lock;
    result = ::py::reinterpret_steal<::py::object>(
      PyByteArray_FromStringAndSize(dataWithSize.first, dataWithSize.second));
  }

  void visitIterator(AnyReference v)
  {
    visitUnknown(v);
  }

  void visitOptional(AnyReference v)
  {
    GILAcquire lock;
    result = unwrapValue(v.content());
  }

  ::py::object& result;
};

/// Singleton for a default constructible type.
template<typename T>
T* instance()
{
  static T instance;
  return &instance;
}

/// Singleton for a non-default constructible type.
template<typename T, typename... Args>
T* instance(Args&&... args)
{
  static boost::synchronized_value<std::map<std::tuple<ka::Decay<Args>...>, T>>
    instances;

  auto syncInstances = instances.synchronize();
  auto it = syncInstances->find(std::forward_as_tuple(args...));
  if (it == syncInstances->end())
  {
    std::tie(it, std::ignore) =
      syncInstances->emplace(std::piecewise_construct,
                             std::forward_as_tuple(args...),
                             std::forward_as_tuple(args...));
  }
  return &it->second;
}

/// A 'disowned' reference would be a reference that is not expected to be automatically destroyed,
/// meaning we would have to manually call destroy on it. Such a storage enables us to associate
/// such references to a type-erased value so that we can later retrieve them and destroy them
/// manually.
using DisownedReferencesStorage =
  boost::synchronized_value<std::map<void*, std::vector<AnyReference>>>;

void storeDisownedReference(void* context, AnyReference ref) noexcept
{
  auto* storage = instance<DisownedReferencesStorage>();
  auto syncStorage = storage->synchronize();
  (*syncStorage)[context].push_back(ref);
}

std::vector<AnyReference> unstoreDisownedReferences(void* context) noexcept
{
  auto* storage = instance<DisownedReferencesStorage>();
  auto syncStorage = storage->synchronize();
  auto it = syncStorage->find(context);
  if (it == syncStorage->end())
    return {};

  std::vector<AnyReference> res;
  swap(it->second, res);
  syncStorage->erase(it);
  return res;
}

std::size_t destroyDisownedReferences(void* context) noexcept
{
  const auto refs = unstoreDisownedReferences(context);
  for (auto ref : refs)
    ref.destroy();
  return refs.size();
}

/// Associates a value to a Python object, so that it shares its lifetime.
template<typename T>
AnyReference associateValueToObj(::py::object& obj, T value)
{
  const auto res = AnyValue::from(std::move(value)).release();
  auto pybindObjPtr = &obj;
  storeDisownedReference(pybindObjPtr, res);

  // Create a weak reference to the target Python object that will track its lifetime and execute a
  // callback once it is destroyed.
  //
  // The callback ensures that all the references associated to the target object are destroyed, so
  // that we can leak them here and still ensure that they will be properly destroyed once needed.
  //
  // The callback also takes a handle to the weakref Python object itself. We need to make sure that
  // this weakref object lives as long as the target Python object so that the callback stays
  // attached to the lifetime of the target object. To do that, we leak the weakref Python object
  // by releasing it, and we manually decrement the reference count in the callback.
  auto weakref = ::py::weakref(obj, ::py::cpp_function([=](::py::handle weakref) {
    destroyDisownedReferences(pybindObjPtr);
    weakref.dec_ref();
  }));
  weakref.release();
  return res;
}

} // namespace

::py::object unwrapValue(AnyReference val)
{
  GILAcquire lock;
  ::py::object result;
  ValueToPyObject tpo(result);
  typeDispatch(tpo, val);
  return result;
}

namespace types
{

template<typename Storage, typename Interface>
class ObjectInterfaceBase : public Interface
{
public:
  static_assert(std::is_base_of<::py::object, ka::RemoveCvRef<Storage>>::value, "");

  Storage* asObjectPtr(void** storage)
  {
    return static_cast<Storage*>(ptrFromStorage(storage));
  }

  Storage& asObject(void** storage)
  {
    return *asObjectPtr(storage);
  }

  void* initializeStorage(void* ptr) override
  {
    if (ptr)
      return ptr;

    GILAcquire lock;
    return new Storage;
  }

  void* clone(void* storage) override
  {
    GILAcquire lock;
    return new Storage(asObject(&storage));
  }

  void destroy(void* storage) override
  {
    destroyDisownedReferences(storage);
    GILAcquire lock;
    delete asObjectPtr(&storage);
  }

  using DefaultImpl = DefaultTypeImplMethods<Storage>;

  void* ptrFromStorage(void** storage) override { return DefaultImpl::ptrFromStorage(storage); }
  const TypeInfo& info() override { return DefaultImpl::info(); }

  bool less(void* a, void* b) override
  {
    GILAcquire lock;
    const auto& objA = asObject(&a);
    const auto& objB = asObject(&b);
    return objA < objB;
  }
};

template<typename Storage = ::py::object>
class DynamicInterface : public ObjectInterfaceBase<Storage, qi::DynamicTypeInterface>
{
  AnyReference get(void* storage) override
  {
    GILAcquire lock;
    const auto obj = this->asObjectPtr(&storage);
    return unwrapAsRef(*obj);
  }

  void set(void** storage, AnyReference src) override
  {
    GILAcquire lock;
    this->asObject(storage) = unwrapValue(src);
  }
};

template<typename Storage = ::py::int_>
class IntInterface : public ObjectInterfaceBase<Storage, qi::IntTypeInterface>
{
  using Repr = long long;

  std::int64_t get(void* storage) override
  {
    GILAcquire lock;
    const auto& obj = this->asObject(&storage);
    return numericConvertBound<std::int64_t>(::py::cast<Repr>(obj));
  }

  void set(void** storage, std::int64_t val) override
  {
    QI_ASSERT_NOT_NULL(storage);
    GILAcquire lock;
    this->asObject(storage) = ::py::int_(static_cast<Repr>(val));
  }

  unsigned int size() override { return sizeof(Repr); }
  bool isSigned() override { return std::is_signed<Repr>::value; }
};

template<typename Storage = ::py::float_>
class FloatInterface : public ObjectInterfaceBase<Storage, qi::FloatTypeInterface>
{
public:
  using Repr = double;

  double get(void* storage) override
  {
    GILAcquire lock;
    const auto& obj = this->asObject(&storage);
    return numericConvertBound<double>(::py::cast<Repr>(obj));
  }

  void set(void** storage, double val) override
  {
    QI_ASSERT_NOT_NULL(storage);
    GILAcquire lock;
    this->asObject(storage) = ::py::float_(static_cast<Repr>(val));
  }

  unsigned int size() override { return sizeof(Repr); }
};

template<typename Storage = ::py::bool_>
class BoolInterface : public ObjectInterfaceBase<Storage, qi::IntTypeInterface>
{
public:
  int64_t get(void* storage) override
  {
    GILAcquire lock;
    const auto& obj = this->asObject(&storage);
    return static_cast<int64_t>(::py::cast<bool>(obj));
  }

  void set(void** storage, int64_t val) override
  {
    QI_ASSERT_NOT_NULL(storage);
    GILAcquire lock;
    this->asObject(storage) = ::py::bool_(static_cast<bool>(val));
  }

  unsigned int size() override { return 0; }
  bool isSigned() override { return false; }
};

template<typename Storage = ::py::str>
class StrInterface : public ObjectInterfaceBase<Storage, qi::StringTypeInterface>
{
public:
  StringTypeInterface::ManagedRawString get(void* storage) override
  {
    GILAcquire lock;
    ::py::str obj = this->asObject(&storage);
    return makeManagedString(std::string(obj));
  }

  void set(void** storage, const char* ptr, size_t sz) override
  {
    GILAcquire lock;
     this->asObject(storage) = ::py::str(ptr, sz);
  }
};

template<typename Storage = ::py::bytes, typename BufferType = ::py::bytes>
class StringBufferInterface : public ObjectInterfaceBase<Storage,
                                                         qi::StringTypeInterface>
{
public:
  StringTypeInterface::ManagedRawString get(void* storage) override
  {
    GILAcquire lock;
    ::py::buffer obj = this->asObject(&storage);
    const auto info = obj.request();
    QI_ASSERT_TRUE(info.ndim == 1);
    QI_ASSERT_TRUE(info.itemsize == sizeof(char));
    return makeManagedString(std::string(static_cast<char*>(info.ptr), info.size));
  }

  void set(void** storage, const char* ptr, size_t sz) override
  {
    GILAcquire lock;
     this->asObject(storage) = ::py::bytes(ptr, sz);
  }
};

template<typename Storage>
class StructuredIterableInterface
  : public ObjectInterfaceBase<Storage, qi::StructTypeInterface>
{
public:
  StructuredIterableInterface(std::size_t size)
    : _size(size)
  {}

  std::vector<TypeInterface*> memberTypes() override
  {
    return std::vector<TypeInterface*>(_size, typeOf<::py::object>());
  }

  std::vector<void*> get(void* storage) override
  {
    GILAcquire lock;
    const auto& obj = this->asObject(&storage);

    std::vector<void*> res;
    res.reserve(_size);
    for (const ::py::handle itemHandle : obj)
    {
      const auto item = ::py::reinterpret_borrow<::py::object>(itemHandle);
      const auto itemRef = AnyValue::from(item).release();
      storeDisownedReference(storage, itemRef);
      res.push_back(itemRef.rawValue());
    }
    return res;
  }

  void* get(void* storage, unsigned int index) override
  {
    QI_ASSERT_TRUE(index < _size);

    GILAcquire lock;
    const auto& obj = this->asObject(&storage);
    // AppleClang 8 wrongly requires a ForwardIterator on `std::next`, which
    // `pybind11::iterator` is not. We use advance instead.
    auto it = obj.begin();
    std::advance(it, index);
    const auto item = ::py::reinterpret_borrow<::py::object>(*it);
    const auto itemRef = AnyValue::from(item).release();
    storeDisownedReference(storage, itemRef);
    return itemRef.rawValue();
  }

  void set(void** /*storage*/, const std::vector<void*>&) override
  {
    throw std::runtime_error("set a python structured iterable object is unimplemented");
  }

  void set(void** /*storage*/, unsigned int /*index*/, void* /*valStorage*/) override
  {
    throw std::runtime_error("set a python structured iterable object is unimplemented");
  }

private:
  std::size_t _size;
};

template<typename Storage = ::py::list, typename ListType = ::py::list>
class ListInterface : public ObjectInterfaceBase<Storage, qi::ListTypeInterface>
{
public:
  using Iterator = std::pair<void* /*list storage*/, unsigned int /*element*/>;

  class IteratorInterface : public qi::IteratorTypeInterface
  {
  public:
    AnyReference dereference(void* storage) override
    {
      const auto& iter = asIter(&storage);
      auto* const listType = instance<ListInterface>();
      auto* listStorage = iter.first;
      const auto index = iter.second;

      GILAcquire lock;
      ListType list = listType->asObject(&listStorage);
      const ::py::object element = list[index];

      auto ref = AnyReference::from(element).clone();
      // Store the disowned reference with the list as a context instead of the
      // iterator because the reference might outlive the iterator.
      storeDisownedReference(listStorage, ref);
      return ref;
    }

    void next(void** storage) override { ++asIter(storage).second; }
    bool equals(void* s1, void* s2) override { return asIter(&s1) == asIter(&s2); }

    using DefaultImpl = DefaultTypeImplMethods<Iterator, TypeByPointerPOD<Iterator>>;

    void* initializeStorage(void* ptr = nullptr) override
    {
      return DefaultImpl::initializeStorage(ptr);
    }

    void* clone(void* storage) override { return DefaultImpl::clone(storage); }
    void destroy(void* storage) override
    {
      destroyDisownedReferences(storage);
      return DefaultImpl::destroy(storage);
    }
    const TypeInfo& info() override { return DefaultImpl::info(); }
    void* ptrFromStorage(void** s) override { return DefaultImpl::ptrFromStorage(s); }
    bool less(void* a, void* b) override { return DefaultImpl::less(a, b); }
    Iterator* asIterPtr(void** storage) { return static_cast<Iterator*>(ptrFromStorage(storage)); }
    Iterator& asIter(void** storage) { return *asIterPtr(storage); }
  };

  DynamicInterface<::py::object>* elementType() override
  {
    return instance<DynamicInterface<::py::object>>();
  }

  size_t size(void* storage) override
  {
    GILAcquire lock;
    ListType list = this->asObject(&storage);
    return list.size();
  }

  void pushBack(void** storage, void* valueStorage) override
  {
    GILAcquire lock;
    ::py::object obj = this->asObject(storage);
    if (::py::isinstance<::py::list>(obj))
    {
      ::py::list list = obj;
      list.append(elementType()->asObject(&valueStorage));
    }
    else
      throw std::runtime_error("cannot append an element on a object that is not a list");
  }

  AnyIterator begin(void* storage) override
  {
    return AnyValue(AnyReference(instance<IteratorInterface>(), new Iterator(storage, 0)),
                    // Do not copy, but free the value, so basically the AnyValue
                    // takes ownership of the object.
                    false, true);
  }

  AnyIterator end(void* storage) override
  {
    return AnyValue(AnyReference(instance<IteratorInterface>(),
                                 new Iterator(storage, size(storage))),
                    // Do not copy, but free the value, so basically the AnyValue
                    // takes ownership of the object.
                    false, true);
  }
};

template<typename Storage>
class DictInterface: public ObjectInterfaceBase<Storage, qi::MapTypeInterface>
{
public:
  using Iterator = std::pair<void* /*dict storage*/, unsigned int /*element*/>;

  class IteratorInterface : public qi::IteratorTypeInterface
  {
  public:
    AnyReference dereference(void* storage) override
    {
      const auto& iter = asIter(&storage);
      auto* const dictType = instance<DictInterface>();
      auto* dictStorage = iter.first;
      const auto index = iter.second;

      GILAcquire lock;
      ::py::dict dict = dictType->asObject(&dictStorage);
      // AppleClang 8 wrongly requires a ForwardIterator on `std::next`, which
      // `pybind11::iterator` is not. We use advance instead.
      auto it = dict.begin();
      std::advance(it, index);
      const auto key = ::py::reinterpret_borrow<::py::object>(it->first);
      const auto element = ::py::reinterpret_borrow<::py::object>(it->second);
      const auto keyElementPair = std::make_pair(key, element);
      auto ref = AnyReference::from(keyElementPair).clone();
      // Store the disowned reference with the list as a context instead of the
      // iterator because the reference might outlive the iterator.
      storeDisownedReference(dictStorage, ref);
      return ref;
    }

    void next(void** storage) override { ++asIter(storage).second; }
    bool equals(void* s1, void* s2) override { return asIter(&s1) == asIter(&s2); }

    using DefaultImpl = DefaultTypeImplMethods<Iterator, TypeByPointerPOD<Iterator>>;

    void* initializeStorage(void* ptr = nullptr) override
    {
      return DefaultImpl::initializeStorage(ptr);
    }

    void* clone(void* storage) override { return DefaultImpl::clone(storage); }
    void destroy(void* storage) override
    {
      destroyDisownedReferences(storage);
      return DefaultImpl::destroy(storage);
    }

    const TypeInfo& info() override { return DefaultImpl::info(); }
    void* ptrFromStorage(void** s) override { return DefaultImpl::ptrFromStorage(s); }
    bool less(void* a, void* b) override { return DefaultImpl::less(a, b); }
    Iterator* asIterPtr(void** storage) { return static_cast<Iterator*>(ptrFromStorage(storage)); }
    Iterator& asIter(void** storage) { return *asIterPtr(storage); }
  };

  DynamicInterface<::py::object>* elementType() override
  {
    return instance<DynamicInterface<::py::object>>();
  }

  DynamicInterface<::py::object>* keyType() override
  {
    return instance<DynamicInterface<::py::object>>();
  }

  size_t size(void* storage) override
  {
    GILAcquire lock;
    ::py::dict dict = this->asObject(&storage);
    return dict.size();
  }

  AnyIterator begin(void* storage) override
  {
    GILAcquire lock;
    ::py::dict dict = this->asObject(&storage);
    return AnyValue(AnyReference(instance<IteratorInterface>(), new Iterator(storage, 0)),
                    // Do not copy, but free the value, so basically the AnyValue
                    // takes ownership of the object.
                    false, true);
  }

  AnyIterator end(void* storage) override
  {
    return AnyValue(AnyReference(instance<IteratorInterface>(),
                                 new Iterator(storage, size(storage))),
                    // Do not copy, but free the value, so basically the AnyValue
                    // takes ownership of the object.
                    false, true);
  }

  void insert(void** storage, void* keyStorage, void* valueStorage) override
  {
    GILAcquire lock;
    ::py::dict dict = this->asObject(storage);
    ::py::object key = keyType()->asObject(&keyStorage);
    ::py::object value = elementType()->asObject(&valueStorage);
    dict[key] = value;
  }

  AnyReference element(void** storage, void* keyStorage, bool autoInsert) override
  {
    GILAcquire lock;
    ::py::dict dict = this->asObject(storage);
    ::py::object key = keyType()->asObject(&keyStorage);

    ::py::object value;
    if (dict.contains(key))
      value = dict[key];
    else
    {
      if (!autoInsert)
        return AnyReference();
      dict[key] = ::py::none();
    }

    auto ref = AnyReference::from(value).clone();
    // Store the disowned reference with the list as a context instead of the
    // iterator because the reference might outlive the iterator.
    storeDisownedReference(storage, ref);
    return ref;
  }
};

} // namespace types

AnyReference unwrapAsRef(pybind11::object& obj)
{
  QI_ASSERT_TRUE(obj);

  GILAcquire lock;

  if (obj.is_none())
    // The "void" value in AnyValue has no storage, so we can just release it
    // without risk of a leak.
    return AnyValue::makeVoid().release();

  if (   ::py::isinstance<::py::ellipsis>(*obj)
      || PyComplex_CheckExact(obj.ptr())
      || ::py::isinstance<::py::memoryview>(*obj)
      || ::py::isinstance<::py::slice>(*obj)
      || ::py::isinstance<::py::module>(*obj))
  {
    throw std::runtime_error("The Python type " +
                             std::string(::py::str(obj.get_type())) +
                             " is not handled");
  }

  // Pointer to the libpython C library python object.
  const auto pyObjPtr = obj.ptr();

  // Pointer to the pybind C++ library python object.
  const auto pybindObjPtr = &obj;

  if (PyLong_CheckExact(pyObjPtr))
    return AnyReference(instance<types::IntInterface<::py::object>>(), pybindObjPtr);

  if (PyFloat_CheckExact(pyObjPtr))
    return AnyReference(instance<types::FloatInterface<::py::object>>(), pybindObjPtr);

  if (PyBool_Check(pyObjPtr))
    return AnyReference(instance<types::BoolInterface<::py::object>>(), pybindObjPtr);

  if (PyUnicode_CheckExact(pyObjPtr))
    return AnyReference(instance<types::StrInterface<::py::object>>(), pybindObjPtr);

  if (PyBytes_CheckExact(pyObjPtr) || PyByteArray_CheckExact(pyObjPtr))
    return AnyReference(instance<types::StringBufferInterface<::py::object>>(), pybindObjPtr);

  if (PyTuple_CheckExact(pyObjPtr))
    return AnyReference(instance<types::StructuredIterableInterface<::py::object>>(
                          ::py::tuple(obj).size()),
                        pybindObjPtr);

  // Checks if it is AnySet, meaning it can be a set or a frozenset.
  if (PyAnySet_CheckExact(pyObjPtr))
    return AnyReference(instance<types::StructuredIterableInterface<::py::object>>(
                          ::py::set(obj).size()),
                        pybindObjPtr);

  if (PyList_CheckExact(pyObjPtr) || PyDictViewSet_Check(pyObjPtr) || PyDictValues_Check(pyObjPtr))
    return AnyReference(instance<types::ListInterface<::py::object, ::py::list>>(), pybindObjPtr);

  if (PyDict_CheckExact(pyObjPtr))
    return AnyReference(instance<types::DictInterface<::py::object>>(), pybindObjPtr);

  // At the moment in libqi, the `LogLevel` type is not registered in the qi type system. If we use
  // `AnyValue::from` with a `LogLevel` value, we get a value with a dummy type that cannot be set
  // or read. Furthermore, when registered with the `QI_TYPE_ENUM` macro, enumeration types are
  // treated as `int` values. This forces us to cast it here as an `int` to try to stay compatible.
  if (::py::isinstance<LogLevel>(obj))
  {
    const auto logLevel = obj.cast<LogLevel>();
    const auto logLevelValue = static_cast<int>(logLevel);
    return associateValueToObj(obj, logLevelValue);
  }

  return associateValueToObj(obj, py::toObject(obj));
}

void registerTypes()
{
  // This list of types is just the most used types and is therefore not
  // exhaustive. At this point these are the only default conversions we need.
  // It doesn't mean these are the only types we support, they're just the only
  // types that will be supported by `qi::typeOf` and automatically by
  // `qi::AnyValue` (for instance through functions like `qi::AnyValue::from`).
  // Other types might be supported by `qi::AnyValue` but their interface will
  // have to be provided manually at construction.
  qi::registerType(qi::typeId<::py::object>(),
                   instance<types::DynamicInterface<::py::object>>());

  qi::registerType(qi::typeId<::py::int_>(),
                   instance<types::IntInterface<::py::int_>>());

  qi::registerType(qi::typeId<::py::float_>(),
                   instance<types::FloatInterface<::py::float_>>());

  qi::registerType(qi::typeId<::py::bool_>(),
                   instance<types::BoolInterface<::py::bool_>>());

  qi::registerType(qi::typeId<::py::str>(),
                   instance<types::StrInterface<::py::str>>());

  qi::registerType(qi::typeId<::py::bytes>(),
                   instance<types::StringBufferInterface<::py::bytes>>());

  // The default type interface for the following types is "dynamic" because
  // we cannot give them a proper interface without an instance of an object
  // from which to get its size.
  qi::registerType(qi::typeId<::py::list>(),
                   instance<types::DynamicInterface<::py::list>>());

  qi::registerType(qi::typeId<::py::dict>(),
                   instance<types::DynamicInterface<::py::dict>>());

  qi::registerType(qi::typeId<::py::kwargs>(),
                   instance<types::DynamicInterface<::py::kwargs>>());

  qi::registerType(qi::typeId<::py::tuple>(),
                   instance<types::DynamicInterface<::py::tuple>>());

  qi::registerType(qi::typeId<::py::args>(),
                   instance<types::DynamicInterface<::py::args>>());
}

} // py
} // qi
