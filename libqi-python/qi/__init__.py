#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" QiMessaging Python bindings """
from __future__ import absolute_import

import os
import sys
import ctypes
import platform
import traceback

PATH_LIBQI = os.path.dirname(os.path.realpath(__file__))

# Add LibQi Python Folder to the Path
sys.path.append(PATH_LIBQI)

# Set Path and Load Dependancies for the Platform
if "aldebaran" in platform.platform():
    path_robot_lib = os.path.join(PATH_LIBQI, "robot")
    sys.path.append(path_robot_lib)
    current_lib_path = os.environ.get("LD_LIBRARY_PATH", "")
    if current_lib_path:
        current_lib_path += ":"
    current_lib_path += ":" + path_robot_lib
    os.environ["LD_LIBRARY_PATH"] = current_lib_path
    robot_dependencies = [
        "libc.so.6",
        "libstdc++.so.6",
        "ld-linux.so.2",
        "librt.so.1",
        "libm.so.6",
        "libdl.so.2",
        "libgcc_s.so.1",
        "libssl.so.1.0.0",
        "libpthread.so.0",
        "libsystemd.so.0",
        "libcrypto.so.1.0.0",
        "libpython2.7.so.1.0",
        "libboost_chrono.so.1.59.0",
        "libboost_date_time.so.1.59.0",
        "libboost_filesystem.so.1.59.0",
        "libboost_locale.so.1.59.0",
        "libboost_program_options.so.1.59.0",
        "libboost_python.so.1.59.0",
        "libboost_regex.so.1.59.0",
        "libboost_system.so.1.59.0",
        "libboost_thread.so.1.59.0",
        "libqi.so",
        "libqipython.so",
    ]
    for dependency in robot_dependencies:
        library_path = os.path.join(PATH_LIBQI, "robot", dependency)
        try:
            ctypes.cdll.LoadLibrary(library_path)
        except:
            print("Unable to load %s\n%s" % (library_path, traceback.format_exc()))
elif sys.platform.startswith("linux"):
    path_linux_lib = os.path.join(PATH_LIBQI, "linux")
    sys.path.append(path_linux_lib)
    current_lib_path = os.environ.get("LD_LIBRARY_PATH", "")
    if current_lib_path:
        current_lib_path += ":"
    current_lib_path += ":" + path_linux_lib
    os.environ["LD_LIBRARY_PATH"] = current_lib_path
    linux_dependencies = [
        "libicudata.so",
        "libicuuc.so",
        "libicui18n.so",
        "libcrypto.so",
        "libssl.so",
        "libpython2.7.so",
        "libboost_system.so",
        "libboost_thread.so",
        "libboost_python.so",
        "libboost_chrono.so",
        "libboost_program_options.so",
        "libboost_filesystem.so",
        "libboost_regex.so",
        "libboost_locale.so",
        "libqi.so",
        "libqipython.so",
    ]
    for dependency in linux_dependencies:
        library_path = os.path.join(PATH_LIBQI, "linux", dependency)
        try:
            ctypes.cdll.LoadLibrary(library_path)
        except:
            print("Unable to load %s\n%s" % (library_path, traceback.format_exc()))
elif sys.platform.startswith("darwin"):
    path_mac_lib = os.path.join(PATH_LIBQI, "mac")
    path_mac_qi = os.path.join(PATH_LIBQI, "mac", "python2.7", "site-packages")
    sys.path.append(path_mac_lib)
    sys.path.append(path_mac_qi)
    current_lib_path = os.environ.get("DYLD_LIBRARY_PATH", "")
    if current_lib_path:
        current_lib_path += ":"
    current_lib_path += ":" + path_mac_lib
    os.environ["DYLD_LIBRARY_PATH"] = current_lib_path
    mac_dependencies = [
        "libcrypto.1.0.0.dylib",
        "libssl.1.0.0.dylib",
        "libboost_system.dylib",
        "libboost_python.dylib",
        "libboost_date_time.dylib",
        "libboost_chrono.dylib",
        "libboost_filesystem.dylib",
        "libboost_regex.dylib",
        "libboost_program_options.dylib",
        "libboost_locale.dylib",
        "libboost_thread.dylib",
        "libqi.dylib",
        "libqipython.dylib",
        "python2.7/site-packages/_qi.so",
    ]
    for dependency in mac_dependencies:
        library_path = os.path.join(PATH_LIBQI, "mac", dependency)
        try:
            ctypes.cdll.LoadLibrary(library_path)
        except:
            print("Unable to load %s\n%s" % (library_path, traceback.format_exc()))
elif sys.platform.startswith("win"):
    sys.path.append(os.path.join(PATH_LIBQI, "win"))
    ctypes.windll.kernel32.SetDllDirectoryA(os.path.join(PATH_LIBQI, "win"))

# Import LibQi Functionnalities
from _qi import Application as _Application
from _qi import ApplicationSession as _ApplicationSession
from _qi import (FutureState, FutureTimeout,
                Future, futureBarrier, Promise,
                Property, Session, Signal,
                async, PeriodicTask)
from _qi import (clockNow, steadyClockNow, systemClockNow)
from _qi import (module, listModules)
from . import path
from ._type import (Void, Bool, Int8, UInt8, Int16, UInt16,
                    Int32, UInt32, Int64, UInt64,
                    Float, Double, String, List,
                    Map, Struct, Object, Dynamic,
                    Buffer, AnyArguments, typeof, _isinstance)
from ._binder import bind, nobind, singleThreaded, multiThreaded
from .logging import fatal, error, warning, info, verbose, Logger
from .logging import getLogger, logFatal, logError, logWarning, logInfo, logVerbose, logDebug  # Deprecated
from .translator import defaultTranslator, tr, Translator
from .version import version

# Set the Version Number
__version__ = version

def PromiseNoop(*args, **kwargs):
    """ No operation function deprecated:: 2.5 """
    pass

# Rename isinstance here. (isinstance should not be used in this file)
isinstance = _isinstance

_app = None

# We want to stop all thread before python start destroying module and the like.
# (this avoid callback calling python while it's destroying)
def _stopApplication():
    """ Stop the Application """
    global _app
    if _app is not None:
        _app.stop()
        del _app
        _app = None

# Application is a singleton, it should live till the end
# of the program because it owns eventloops
def Application(args=None, raw=False, autoExit=True, url=None):
    """ Instanciate and return the App """
    global _app
    if _app is None:
        if args is None:
            args = sys.argv
        if url is None:
            url = "tcp://127.0.0.1:9559"
        if not args:
            args = ['python']
        elif args[0] == '':
            args[0] = 'python'
        if raw:
            _app = _Application(args)
        else:
            _app = _ApplicationSession(args, autoExit, url)
    else:
        raise Exception("Application was already initialized")
    return _app

ApplicationSession = Application

__all__ = [
    "FutureState",
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
    "getLogger", "logFatal", "logError", "logWarning", "logInfo", "logVerbose", "logDebug", # Deprecated
    "Logger", "defaultTranslator", "tr", "Translator",
    "module", "listModules",
    "clockNow", "steadyClockNow", "systemClockNow"
]

# Register _stopApplication as a function to be executed at termination
import atexit
atexit.register(_stopApplication)
del atexit
