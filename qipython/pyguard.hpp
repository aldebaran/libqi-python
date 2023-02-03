/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_GUARD_HPP
#define QIPYTHON_GUARD_HPP

#include <qipython/common.hpp>
#include <ka/typetraits.hpp>
#include <type_traits>

namespace qi
{
namespace py
{

/// Returns whether or not the GIL is currently held by the current thread.
inline bool currentThreadHoldsGil()
{
  return PyGILState_Check() == 1;
}

/// DefaultConstructible Guard
/// Procedure<_ (Args)> F
template<typename Guard, typename F, typename... Args>
ka::ResultOf<F(Args&&...)> invokeGuarded(F&& f, Args&&... args)
{
  Guard g;
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

namespace detail
{

// Guards the copy, construction and destruction of an object, and only when
// in case of copy construction the source object evaluates to true, and in case
// of destruction, the object itself evaluates to true.
//
// Explanation:
//   pybind11::object only requires the GIL to be locked on copy and if the
//   source is not null. Default construction does not require the GIL either.
template<typename Guard, typename Object>
class Guarded
{
  using Storage =
    typename std::aligned_storage<sizeof(Object), alignof(Object)>::type;
  Storage _object;

  Object& object() { return reinterpret_cast<Object&>(_object); }
  const Object& object() const { return reinterpret_cast<const Object&>(_object); }

public:
  template<typename Arg0, typename... Args,
           ka::EnableIf<std::is_constructible<Object, Arg0&&, Args&&...>::value, int> = 0>
  explicit Guarded(Arg0&& arg0, Args&&... args)
  {
    Guard g;
    new(&_object) Object(std::forward<Arg0>(arg0), std::forward<Args>(args)...);
  }

  Guarded()
  {
    // Default construction, no GIL needed.
    new(&_object) Object();
  }

  Guarded(Guarded& o)
  {
    boost::optional<Guard> optGuard;
    if (o.object())
      optGuard.emplace();
    new(&_object) Object(o.object());
  }

  Guarded(const Guarded& o)
  {
    boost::optional<Guard> optGuard;
    if (o.object())
      optGuard.emplace();
    new(&_object) Object(o.object());
  }

  Guarded(Guarded&& o)
  {
    new(&_object) Object(std::move(o.object()));
  }

  ~Guarded()
  {
    boost::optional<Guard> optGuard;
    if (object())
      optGuard.emplace();
    object().~Object();
  }

  Guarded& operator=(const Guarded& o)
  {
    if (this == &o)
      return *this;

    {
      boost::optional<Guard> optGuard;
      if (o.object())
        optGuard.emplace();
      object() = o.object();
    }
    return *this;
  }

  Guarded& operator=(Guarded&& o)
  {
    if (this == &o)
      return *this;

    object() = std::move(o.object());
    return *this;
  }

  Object& operator*() { return object(); }
  const Object& operator*() const { return object(); }
  Object* operator->() { return &object(); }
  const Object* operator->() const { return &object(); }
  explicit operator Object&() { return object(); }
  explicit operator const Object&() const { return object(); }
};

/// G == pybind11::gil_scoped_acquire || G == pybind11::gil_scoped_release
template<typename G>
void pybind11GuardDisarm(G& guard)
{
  QI_IGNORE_UNUSED(guard);
// The disarm API was introduced in v2.6.2.
#if QI_CURRENT_PYBIND11_VERSION >= QI_PYBIND11_VERSION(2,6,2)
  guard.disarm();
#endif
}

} // namespace detail

/// RAII utility type that guarantees that the GIL is locked for the scope of
/// the lifetime of the object.
///
/// Objects of this type (or objects composed of them) must not be kept alive
/// after the hand is given back to the interpreter.
///
/// This type is re-entrant.
///
/// postcondition: `GILAcquire acq;` establishes
/// `(interpreterIsFinalizing() && *interpreterIsFinalizing()) || currentThreadHoldsGil()`
struct GILAcquire
{
  inline GILAcquire()
  {
    const auto optIsFinalizing = interpreterIsFinalizing();
    const auto definitelyFinalizing = optIsFinalizing && *optIsFinalizing;
    // `gil_scoped_acquire` is re-entrant by itself, so we don't need to check
    // whether or not the GIL is already held by the current thread.
    if (!definitelyFinalizing)
      _acq.emplace();
    QI_ASSERT(definitelyFinalizing || currentThreadHoldsGil());
  }

  GILAcquire(const GILAcquire&) = delete;
  GILAcquire& operator=(const GILAcquire&) = delete;

private:
  boost::optional<pybind11::gil_scoped_acquire> _acq;
};

/// RAII utility type that guarantees that the GIL is unlocked for the scope of
/// the lifetime of the object.
///
/// Objects of this type (or objects composed of them) must not be kept alive
/// after the hand is given back to the interpreter.
///
/// This type is re-entrant.
///
/// postcondition: `GILRelease rel;` establishes
/// `(interpreterIsFinalizing() && *interpreterIsFinalizing()) || !currentThreadHoldsGil()`
struct GILRelease
{
  inline GILRelease()
  {
    const auto optIsFinalizing = interpreterIsFinalizing();
    const auto definitelyFinalizing = optIsFinalizing && *optIsFinalizing;
    if (!definitelyFinalizing && currentThreadHoldsGil())
      _release.emplace();
    QI_ASSERT(definitelyFinalizing || !currentThreadHoldsGil());
  }

  GILRelease(const GILRelease&) = delete;
  GILRelease& operator=(const GILRelease&) = delete;

private:
  boost::optional<pybind11::gil_scoped_release> _release;
};

/// Wraps a pybind11::object value and locks the GIL on copy and destruction.
///
/// This is useful for instance to put pybind11 objects in lambda functions so
/// that they can be copied around safely.
using GILGuardedObject = detail::Guarded<GILAcquire, pybind11::object>;

/// Deleter that deletes the pointer outside the GIL.
///
/// Useful for types that might deadlock on destruction if they keep the GIL
/// locked.
struct DeleteOutsideGIL
{
  template<typename T>
  void operator()(T* ptr) const
  {
    GILRelease unlock;
    delete ptr;
  }
};

/// Deleter that delays the destruction of an object to another thread.
struct DeleteInOtherThread
{
  template<typename T>
  void operator()(T* ptr) const
  {
    GILRelease unlock;
    // `std::async` returns an object, that unless moved from will block when
    // destroyed until the task is complete, which is unwanted behavior here.
    // Therefore we just take the result of the `std::async` call into a local
    // variable and then ignore it.
    auto fut = std::async(std::launch::async, [](std::unique_ptr<T>) {},
                          std::unique_ptr<T>(ptr));
    QI_IGNORE_UNUSED(fut);
  }
};

} // namespace py
} // namespace qi

#endif // QIPYTHON_GUARD_HPP
