#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" QiMessaging Python bindings """
from __future__ import absolute_import
from __future__ import print_function

import os
import sys
import ctypes
import platform
import traceback

LOAD_DEPEDENCIES = {
    "robot": [
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
        "libboost_chrono.so.1.64.0",
        "libboost_date_time.so.1.64.0",
        "libboost_filesystem.so.1.64.0",
        "libboost_locale.so.1.64.0",
        "libboost_program_options.so.1.64.0",
        "libboost_python.so.1.64.0",
        "libboost_regex.so.1.64.0",
        "libboost_system.so.1.64.0",
        "libboost_thread.so.1.64.0",
        "libboost_random.so.1.64.0",
        "libqi.so",
        "libqipython.so",
    ],
    "linux": [
        "libicudata.so.55",
        "libicuuc.so.55",
        "libicui18n.so.55",
        "libboost_system.so.1.64.0",
        "libboost_thread.so.1.64.0",
        "libboost_python.so.1.64.0",
        "libboost_chrono.so.1.64.0",
        "libboost_date_time.so.1.64.0",
        "libboost_program_options.so.1.64.0",
        "libboost_filesystem.so.1.64.0",
        "libboost_regex.so.1.64.0",
        "libboost_locale.so.1.64.0",
        "libboost_random.so.1.64.0",
        "libqi.so",
        "libqipython.so",
    ],
    "mac": [
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
        "libboost_random.dylib",
        "libqi.dylib",
        "libqipython.dylib",
        "python2.7/site-packages/_qi.so",
    ],
    "win": [],
}
PATH_LIBQI_PYTHON = os.path.dirname(os.path.realpath(__file__))


def check_in_path(envname, addpath):
    """ Check that Path is in an Evironment Variable """
    path_envname = os.environ.get(envname, "")
    if addpath not in path_envname:
        if path_envname:
            path_envname += os.path.pathsep
        path_envname += addpath
        os.environ[envname] = path_envname


def init_platform(name):
    """ Initialize LibQi Python for a Platform and return the Libraries Path """
    # Add LibQi Python to the Path if needed
    if PATH_LIBQI_PYTHON not in sys.path:
        sys.path.append(PATH_LIBQI_PYTHON)
    # Deduce the Platform Path
    platform_path = os.path.join(PATH_LIBQI_PYTHON, name)
    # Add it to the PATH if not present
    if platform_path not in sys.path:
        sys.path.append(platform_path)
    # Update QI_ADDITIONAL_SDK_PREFIXES if needed
    check_in_path("QI_ADDITIONAL_SDK_PREFIXES", platform_path)
    # Register QiCore
    try:
        import qicore
        qicore.register()
    except Exception:
        # Add the QiCore Supposed Path to QI_ADDITIONAL_SDK_PREFIXES
        path_qicore = os.path.join(PATH_LIBQI_PYTHON, "..", "qicore", name)
        check_in_path("QI_ADDITIONAL_SDK_PREFIXES", path_qicore)
    # Register QiGeometry
    try:
        import qigeometry
        qigeometry.register()
    except Exception:
        # Add the QiGeometry Supposed Path to QI_ADDITIONAL_SDK_PREFIXES
        path_qigeometry = os.path.join(PATH_LIBQI_PYTHON, "..", "qigeometry", name)
        check_in_path("QI_ADDITIONAL_SDK_PREFIXES", path_qigeometry)
    # Load the Platform Dependencies
    for library in LOAD_DEPEDENCIES.get(name, []):
        library_path = os.path.join(platform_path, library)
        try:
            ctypes.cdll.LoadLibrary(library_path)
        except:
            print("Unable to load %s\n%s" % (library_path, traceback.format_exc()))
    # Return the Platform Path for further Treatments
    return platform_path


# Launch Initialization for the Platform
if "aldebaran" in platform.platform():
    platform_path = init_platform("robot")
    check_in_path("LD_LIBRARY_PATH", platform_path)
elif sys.platform.startswith("linux"):
    platform_path = init_platform("linux")
    check_in_path("LD_LIBRARY_PATH", platform_path)
elif sys.platform.startswith("darwin"):
    platform_path = init_platform("mac")
    check_in_path("DYLD_LIBRARY_PATH", platform_path)
    path_package = os.path.join(platform_path, "python2.7", "site-packages")
    if path_package not in sys.path:
        sys.path.append(path_package)
elif sys.platform.startswith("win"):
    platform_path = init_platform("win")
    ctypes.windll.kernel32.SetDllDirectoryA(platform_path)

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
            url = ''
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
