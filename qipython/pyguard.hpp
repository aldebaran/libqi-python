/*
**  Copyright (C) 2020 SoftBank Robotics Europe
**  See COPYING for the license
*/

#pragma once

#ifndef QIPYTHON_GUARD_HPP
#define QIPYTHON_GUARD_HPP

#include <qipython/common.hpp>
#include <ka/typetraits.hpp>
#include <pybind11/pybind11.h>
#include <type_traits>

namespace qi
{
namespace py
{

/// Returns whether or not the GIL is currently held by the current thread. If
/// the interpreter is not yet initialized or has been finalized, returns an
/// empty optional, as there is no GIL available.
inline boost::optional<bool> currentThreadHoldsGil()
{
  // PyGILState_Check() returns 1 (success) before the creation of the GIL and
  // after the destruction of the GIL.
  if (Py_IsInitialized() == 1) {
    const auto gilAcquired = PyGILState_Check() == 1;
    return boost::make_optional(gilAcquired);
  }
  return boost::none;
}

/// Returns whether or not the GIL exists (i.e the interpreter is initialized
/// and not finalizing) and is currently held by the current thread.
inline bool gilExistsAndCurrentThreadHoldsIt()
{
  return currentThreadHoldsGil().value_or(false);
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
/// the lifetime of the object. If the GIL cannot be acquired (for example,
/// because the interpreter is finalizing), throws an `InterpreterFinalizingException`
/// exception.
///
/// Objects of this type (or objects composed of them) must not be kept alive
/// after the hand is given back to the interpreter.
///
/// This type is re-entrant.
///
/// postcondition: `GILAcquire acq;` establishes `gilExistsAndCurrentThreadHoldsIt()`
struct GILAcquire
{
  inline GILAcquire()
  {
    if (gilExistsAndCurrentThreadHoldsIt())
      return;

    const auto isFinalizing = interpreterIsFinalizing().value_or(false);
    if (isFinalizing)
      throw InterpreterFinalizingException();

    _state = ::PyGILState_Ensure();
    QI_ASSERT(gilExistsAndCurrentThreadHoldsIt());
  }

  inline ~GILAcquire()
  {
    // Even if releasing the GIL while the interpreter is finalizing is allowed, it does
    // require the GIL to be currently held. But we have no guarantee that this is the case,
    // because the GIL may have been released since we acquired it, and we could not
    // reacquire it after that (maybe the interpreter is in fact finalizing).
    // Therefore, only release the GIL if it is currently held by this thread.
    if (_state && gilExistsAndCurrentThreadHoldsIt())
      ::PyGILState_Release(*_state);
  }


  GILAcquire(const GILAcquire&) = delete;
  GILAcquire& operator=(const GILAcquire&) = delete;

private:
  boost::optional<PyGILState_STATE> _state;
};

/// RAII utility type that (as a best effort) tries to ensure that the GIL is
/// unlocked for the scope of the lifetime of the object.
///
/// Objects of this type (or objects composed of them) must not be kept alive
/// after the hand is given back to the interpreter.
///
/// This type is re-entrant.
///
/// postcondition: `GILRelease rel;` establishes
///   `interpreterIsFinalizing().value_or(false) ||
///    !gilExistsAndCurrentThreadHoldsIt()`
struct GILRelease
{
  inline GILRelease()
  {
    // Even if releasing the GIL while the interpreter is finalizing is allowed,
    // it does require the GIL to be currently held. However, reacquiring the
    // GIL is forbidden if finalization started.
    //
    // It may happen that we try to release the GIL from a function ('F') called
    // by the Python interpreter, while it is holding the GIL. In this case, if
    // we try to release it, then fail to reacquire it and return the hand to
    // the interpreter, the interpreter will most likely terminate the process,
    // as it expects to still be holding the GIL. Failure to reacquire the GIL
    // can only happen if the interpreter started finalization, either before we
    // released it, or while it was released.
    //
    // Fortunately, in this case, because the interpreter is busy executing the
    // function 'F', it is not possible for it to start finalization until 'F'
    // returns. This means that the only possible reason of failure of
    // reacquisition of the GIL is because the interpreter was finalizing
    // *before* calling 'F', therefore before we released to GIL. Therefore, to
    // prevent this failure, we check if the interpreter is finalizing before
    // releasing the GIL, and if it is, we do nothing, and the GIL stays held.
    const auto isFinalizing = interpreterIsFinalizing().value_or(false);
    if (!isFinalizing && gilExistsAndCurrentThreadHoldsIt())
      _release.emplace();
    QI_ASSERT(isFinalizing || !gilExistsAndCurrentThreadHoldsIt());
  }

  inline ~GILRelease()
  {
    // Reacquiring the GIL is forbidden when the interpreter is finalizing, as
    // it may terminate the current thread.
    const auto isFinalizing = interpreterIsFinalizing().value_or(false);
    if (_release && isFinalizing)
      detail::pybind11GuardDisarm(*_release);
  }

  GILRelease(const GILRelease&) = delete;
  GILRelease& operator=(const GILRelease&) = delete;

private:
  boost::optional<pybind11::gil_scoped_release> _release;
};

/// Wraps a Python object as a shared reference-counted value that does not
/// require the GIL to copy, move or assign to.
///
/// On destruction, it releases the object with the GIL acquired. However, if
/// the GIL cannot be acquired, the object is leaked. This is most likely
/// mitigated by the fact that if the GIL is not available, it means the object
/// already has been or soon will be garbage collected by interpreter
/// finalization.
template<typename T>
class SharedObject
{
  static_assert(std::is_base_of_v<pybind11::object, T>,
                "template parameter T must be a subclass of pybind11::object");

  struct State
  {
    std::mutex mutex;
    T object;
  };
  std::shared_ptr<State> _state;

  struct StateDeleter
  {
    inline void operator()(State* state) const
    {
      auto handle = state->object.release();
      delete state;

      // Do not lock the GIL if there is nothing to release.
      if (!handle)
        return;

      try
      {
        GILAcquire acquire;
        handle.dec_ref();
      }
      catch (const qi::py::InterpreterFinalizingException&)
      {
        // Nothing, the interpreter is finalizing.
      }
    }
  };

public:
  SharedObject() = default;

  inline explicit SharedObject(T object)
    : _state(new State { std::mutex(), std::move(object) }, StateDeleter())
  {
  }

  /// Copies the inner Python object value by incrementing its reference count.
  ///
  /// @pre: If the inner value is not null, the GIL must be acquired.
  T inner() const
  {
    QI_ASSERT_NOT_NULL(_state);
    std::scoped_lock<std::mutex> lock(_state->mutex);
    return _state->object;
  }

  /// Takes the inner Python object value and leaves a null value in its place.
  ///
  /// Any copy of the shared object will now have a null inner value.
  ///
  /// This operation does not require the GIL as the reference count of the
  /// object is preserved.
  T takeInner()
  {
    QI_ASSERT_NOT_NULL(_state);
    std::scoped_lock<std::mutex> lock(_state->mutex);
    /// Hypothesis: Moving a `pybind11::object` steals the object and preserves
    /// its reference count and therefore does not require the GIL.
    return ka::exchange(_state->object, {});
  }
};

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

} // namespace py
} // namespace qi

#endif // QIPYTHON_GUARD_HPP
