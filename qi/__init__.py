#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

"""LibQi Python bindings."""

import platform
py_version = platform.python_version_tuple()
if py_version < ('3', '5'):
    raise RuntimeError('Python version 3.5+ is required.')

import sys  # noqa: E402
import atexit  # noqa: E402

from .qi_python \
  import (FutureState, FutureTimeout, Future, futureBarrier,  # noqa: E402
          Promise, Property, Session, Signal, runAsync, PeriodicTask,
          clockNow, steadyClockNow, systemClockNow, module, listModules,
          Application as _Application,
          ApplicationSession as _ApplicationSession)
from . import path  # noqa: E402
from ._type import (Void, Bool, Int8, UInt8, Int16, UInt16,  # noqa: E402
                    Int32, UInt32, Int64, UInt64,
                    Float, Double, String, List,
                    Optional, Map, Struct, Object, Dynamic,
                    Buffer, AnyArguments, typeof, _isinstance)  # noqa: E402
from ._binder import bind, nobind, singleThreaded, multiThreaded  # noqa: E402
from .logging import fatal, error, warning, info, verbose, Logger  # noqa: E402
from .translator import defaultTranslator, tr, Translator  # noqa: E402
from ._version import __version__  # noqa: E402, F401


__all__ = [
    'FutureState', 'FutureTimeout', 'Future', 'futureBarrier', 'Promise',
    'Property', 'Session', 'Signal', 'runAsync', 'PeriodicTask', 'clockNow',
    'steadyClockNow', 'systemClockNow', 'module', 'listModules',
    'path', 'Void', 'Bool', 'Int8', 'UInt8', 'Int16', 'UInt16', 'Int32',
    'UInt32', 'Int64', 'UInt64', 'Float', 'Double', 'String', 'List', 'Optional',
    'Map', 'Struct', 'Object', 'Dynamic', 'Buffer', 'AnyArguments', 'typeof',
    'isinstance', 'bind', 'nobind', 'singleThreaded', 'multiThreaded', 'fatal',
    'error', 'warning', 'info', 'verbose', 'Logger', 'defaultTranslator', 'tr',
    'Translator', 'Application', 'ApplicationSession'
]

def PromiseNoop(*args, **kwargs):
    """No operation function
    .. deprecated:: 1.5.0"""
    pass


isinstance = _isinstance

_app = None


# We want to stop all threads before Python start destroying module and the
# like (this avoids callbacks calling Python while it's destroying).
def _stop_application():
    global _app
    if _app is not None:
        _app.stop()
        del _app
        _app = None


# Register _stop_application as a function to be executed at termination
atexit.register(_stop_application)


# Application is a singleton, it should live till the end
# of the program because it owns eventloops
def Application(args=None, raw=False, autoExit=True, url=None):
    """Instantiates and returns the Application instance."""
    global _app
    if _app is None:
        if args is None:
            args = sys.argv
        if url is None:
            url = ''
        if not args:
            args = [sys.executable]
        elif args[0] == '':
            args[0] = sys.executable
        if raw:
            _app = _Application(args)
        else:
            _app = _ApplicationSession(args, autoExit, url)
    else:
        raise Exception("Application was already initialized")
    return _app


ApplicationSession = Application

# Retrocompatibility with older versions of the module where `runAsync` was
# named `async`.
if sys.version_info < (3,7):
    globals()['async'] = runAsync
    __all__.append('async')
