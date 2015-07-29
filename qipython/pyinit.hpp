#pragma once
/*
**  Copyright (C) 2013 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QIPYTHON_PYINIT_HPP_
#define _QIPYTHON_PYINIT_HPP_

#include <qipython/api.hpp>
#include <qi/macro.hpp>

namespace qi {
  namespace py {
    /// Initialize Python and release the lock. This method is *not*
    /// threadsafe. Initialising multiple time is safe, but this must not be
    /// called if initialisation has already been done somewhere else.
    QIPYTHON_API void initialize(bool autoUninitialization = true);

    /// Deinitialize Python. This method is automatically called when
    /// qi::Application exits.
    QIPYTHON_API void uninitialize();

    // Deprecated stuff
    /// Initialise Python and release the lock. This method is *not*
    /// threadsafe. Initialising multiple time is safe, but this must not be
    /// called if initialisation has already been done somewhere else.
    /// @deprecated Since 2.5, please call initialize() instead.
    QI_API_DEPRECATED QIPYTHON_API void initialise();

    /// Deinitialise Python. This method is automatically called when
    /// qi::Application exits.
    /// @deprecated Since 2.5, please call uninitialize() instead.
    QI_API_DEPRECATED QIPYTHON_API void uninitialise();
  }
}

#endif  // _QIPYTHON_PYINIT_HPP_
