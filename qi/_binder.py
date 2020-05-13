#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" QiMessaging Python bindings """
from __future__ import absolute_import
# from __future__ import unicode_literals

import inspect

from ._type import AnyArguments

class bind(object):
    """ Allows specifying types and methodName for bound methods """

    def __init__(self, returnType=None, paramsType=None, methodName=None):
        """ Bind Constructor """
        if returnType is None:
            self._retsig = None
        else:
            self._retsig = str(returnType)
        if paramsType is None:
            self._sig = None
        elif isinstance(paramsType, (list, tuple)):
            self._sig = "(%s)" % "".join([str(x) for x in paramsType])
        elif isinstance(paramsType, AnyArguments) or (inspect.isclass(paramsType) and issubclass(paramsType, AnyArguments)):
            self._sig = "m"
        else:
            raise Exception("Invalid types for parameters")
        self._name = methodName

    def __call__(self, f):
        """ Function Generator """
        f.__qi_name__ = self._name
        f.__qi_signature__ = self._sig
        f.__qi_return_signature__ = self._retsig
        return f

def nobind(func):
    """ This function decorator will prevent the function from being bound. (exported) """
    func.__qi_signature__ = "DONOTBIND"
    return func

class singleThreaded(object):
    """
        This class decorator specifies that some methods of this class will
        never be called at the same time on the same instance by the qi module,
        by doing the calls sequentially and ensuring thread safety without the
        need of some extra synchronisation mechanism.
        This guarantee only applies to method calls that originate from the qi
        module, which mostly concerns bound methods and methods connected as
        callbacks of signals.
        It does not apply to private methods (methods that start with
        '__'), including but not restricted to __init__, __del__, __enter__ and
        __exit__.
        One consequence of this is that a sequenced method call on an object
        must finish before another sequenced method call on the same object can
        be made.
        This is the default behavior.
    """

    def __init(self, _):
        pass

    def __call__(self, f):
        """ Function Generator """
        f.__qi_threading__ = "single"
        return f

class multiThreaded(object):
    """
        This class decorator specifies that methods in the class are allowed to be called concurrently.
        This implies that the developer of the class must garantee no concurrent access to it's
        internal data, usually by using some synchronization mechanism.
    """

    def __init(self, _):
        pass

    def __call__(self, f):
        """ Function Generator. """
        f.__qi_threading__ = "multi"
        return f
