##
## Author(s):
##  - Cedric GESTES <gestes@aldebaran-robotics.com>
##  - Pierre ROULLON <proullon@aldebaran-robotics.com>
##
## Copyright (C) 2010 - 2014 Aldebaran Robotics
##

""" QiMessaging Python bindings

Provided features are very close to C++, Python style.
"""

import ctypes
import os
import sys

def get_qilib_path():
    import imp
    fp, pathname, description = imp.find_module('_qi')
    fp.close()
    return pathname

def load_libqipython():
    """ Load _qipyessaging.so and its dependencies.

    This makes _qipyessaging usable from a relocatable
    SDK without having to set LD_LIBRARY_PATH
    """
    deps = [
            "libicudata.so",
            "libicuuc.so",
            "libicui18n.so",
            "libboost_python.so",
            "libboost_system.so",
            "libboost_chrono.so",
            "libboost_program_options.so",
            "libboost_thread.so",
            "libboost_filesystem.so",
            "libboost_regex.so",
            "libboost_locale.so",
            "libboost_signals.so",
            "libqi.so",
    ]
    relpaths = [
            # in pynaoqi, we find /_qi.so and we search for /libqi.so
            [],
            # in deploys/packages/etc,
            # we find $PREFIX/lib/python2.7/site-packages/_qi.so
            # and we need $PREFIX/lib/libqi.so
            ["..", ".."],
            ]
    if sys.version_info[0] == 2:
        deps.append("libqipython.so")
    else:
        deps.append("libqipython3.so")
    qilib = get_qilib_path()
    this_dir = os.path.abspath(os.path.dirname(qilib))
    for dep in deps:
        for relpath in relpaths:
            list_path = [this_dir] + relpath + [dep]
            full_path = os.path.join(*list_path)
            try:
                ctypes.cdll.LoadLibrary(full_path)
                break
            except Exception:
                pass

def set_dll_directory():
    qilib = get_qilib_path()
    this_dir = os.path.dirname(qilib)
    sdk_dir = os.path.join(this_dir, "..", "..")
    sdk_dir = os.path.abspath(sdk_dir)
    bin_dir = os.path.join(sdk_dir, "bin")
    if os.path.exists(bin_dir):
        ctypes.windll.kernel32.SetDllDirectoryA(bin_dir)

def _on_import_module():
    if sys.platform.startswith("linux"):
        load_libqipython()
    if sys.platform.startswith("win"):
        set_dll_directory()


#######
_on_import_module()

from _qi import Application as _Application
from _qi import ApplicationSession as _ApplicationSession
from _qi import ( FutureState, FutureTimeout, Future, futureBarrier, Promise,
                  Property, Session, Signal,
                  async, PeriodicTask)
from _qi import ( clockNow, steadyClockNow, systemClockNow )
from _qi import ( module, listModules )
from . import path
from ._type import ( Void, Bool,
                     Int8, UInt8,
                     Int16, UInt16,
                     Int32, UInt32,
                     Int64, UInt64,
                     Float, Double,
                     String, List,
                     Map, Struct,
                     Object, Dynamic,
                     Buffer, AnyArguments,
                     typeof, _isinstance)
from ._binder import bind, nobind, singleThreaded, multiThreaded
from .logging import fatal, error, warning, info, verbose, Logger
from .logging import getLogger, logFatal, logError, logWarning, logInfo, logVerbose, logDebug  #deprecated
from .translator import defaultTranslator, tr, Translator

def PromiseNoop(*args, **kwargs):
    """No operation function for use with Promise to make it cancellable
    without doing anything special"""
    pass

#rename isinstance here. (isinstance should not be used in this file)
isinstance = _isinstance

_app = None


#we want to stop all thread before python start destroying
#module and the like. (this avoid callback calling python while
#it's destroying)
def _stopApplication():
    global _app
    if _app is not None:
        _app.stop()
        del _app
        _app = None

#application is a singleton, it should live till the end of the program
#because it owns eventloops
def Application(args=None, raw=False, autoExit=True, url=None):
    import sys
    global _app
    if _app is None:
        if args is None:
            args = sys.argv
        if url is None:
            url = "tcp://127.0.0.1:9559"
        if len(args) == 0:
            args = ['python']
        elif args[0] == '':
            args[0] = 'python'
        if raw:
            _app = _Application(args)
        else:
            _app = _ApplicationSession(args, autoExit, url);
    else:
        raise Exception("Application was already initialized")
    return _app

ApplicationSession = Application

__all__ = ["FutureState",
           "FutureTimeout",
           "Future",
           "futureBarrier",
           "Promise",
           "PromiseNoop",
           "Property",
           "Session",
           "Signal",
           "createObject",
           "registerObjectFactory",
           "async",
           "Void", "Bool", "Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32", "Int64", "UInt64",
           "Float", "Double", "String", "List", "Map", "Struct", "Object", "Dynamic", "Buffer", "AnyArguments",
           "typeof", "isinstance",
           "bind", "nobind", "singleThreaded", "multiThreaded",
           "fatal", "error", "warning", "info", "verbose",
           "getLogger", "logFatal", "logError", "logWarning", "logInfo", "logVerbose", "logDebug",  #deprecated
           "Logger", "defaultTranslator", "tr", "Translator",
           "module", "listModules",
           "clockNow", "steadyClockNow", "systemClockNow"
]

import atexit
atexit.register(_stopApplication)
# Do not pollute namespace
del atexit
