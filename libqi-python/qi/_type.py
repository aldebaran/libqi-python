#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" QiMessaging Python bindings """
from __future__ import absolute_import
# from __future__ import unicode_literals

import _qi

class _MetaSignature(type):
    """ MetaSignature """
    def __str__(self):
        return self.signature
    def __unicode__(self):
        return self.signature
    # Support comparing class and instance (Int8 == Int8())
    def __eq__(self, other):
        if isinstance(other, str):
            return other == self.signature
        return other.signature == self.signature
    def __ne__(self, other):
        if isinstance(other, str):
            return other != self.signature
        return other.signature != self.signature

# This syntax works for defining metaclass in python2 and python3
_ToInheritMetaSignature = _MetaSignature('_ToInheritMetaSignature', (object, ), {})

class _Signature(_ToInheritMetaSignature):
    def __str__(self):
        return self.signature
    def __unicode__(self):
        return self.signature
    def __eq__(self, other):
        if isinstance(other, str):
            return other == self.signature()
        return other.signature == self.signature
    def __ne__(self, other):
        return not self.__eq__(other)

class Void(_Signature):
    """ Void Type """
    signature = 'v'

class Bool(_Signature):
    """ Bool Type """
    signature = 'b'

class Int8(_Signature):
    """ Signed 8 bits Integer Type """
    signature = 'c'

class UInt8(_Signature):
    """ Unsigned 8 bits Integer Type """
    signature = 'C'

class Int16(_Signature):
    """ Signed 16 bits Integer Type """
    signature = 'w'

class UInt16(_Signature):
    """ Unsigned 16 bits Integer Type """
    signature = 'W'

class Int32(_Signature):
    """ Signed 32 bits Integer Type """
    signature = 'i'

class UInt32(_Signature):
    """ Unsigned 32 bits Integer Type """
    signature = 'I'

class Int64(_Signature):
    """ Signed 64 bits Integer Type """
    signature = 'l'

class UInt64(_Signature):
    """ Unsigned 64 bits Integer Type """
    signature = 'L'

class Float(_Signature):
    """ 32 bits Floating Point Type """
    signature = 'f'

class Double(_Signature):
    """ 64 bits Floating Point Type """
    signature = 'd'

class String(_Signature):
    """ String Type """
    __metaclass__ = _MetaSignature
    signature = "s"

class List(_Signature):
    """ List Type, an element type need to be specified """
    def __init__(self, elementType):
        self.signature = "[%s]" % elementType.signature

class Map(_Signature):
    """ List Type, a key and an element type need to be specified """
    def __init__(self, keyType, elementType):
        self.signature = "{%s%s}" % (keyType.signature, elementType.signature)

class Struct(_Signature):
    """ Structure Type """
    def __init__(self, fields):
        self.signature = "(%s)" % fields.join("")

class Object(_Signature):
    """ Object Type """
    __metaclass__ = _MetaSignature
    signature = 'o'

class Dynamic(_Signature):
    """ Any Type """
    __metaclass__ = _MetaSignature
    signature = 'm'

class Buffer(_Signature):
    """ Buffer Type """
    __metaclass__ = _MetaSignature
    signature = 'r'

# Yes this look similar to Dynamic but it's not.
# eg: qi.bind(Void, (Dynamic, Dynamic))  this mean a tuple of two dynamic.
# eg: qi.bind(Void, AnyArguments)        this is not a tuple. (m not in tuple, mean anythings)
# eg: qi.bind(Void, Dynamic)             this is a function with one argument
class AnyArguments(_Signature):
    """ Any Arguments Types. A function or a signal taking AnyArguments
        will accept all kind of arguments. AnyArguments is a list of AnyValue
    """
    __metaclass__ = _MetaSignature
    signature = 'm'

# Return the qi.type of the parameter
def typeof(a):
    """ return the qi type of a variable
        .. warning::
           this function is only implemented for Object
    """
    if isinstance(a, _qi.Object):
        return Object
    raise NotImplementedError("typeOf is only implemented for Object right now")

# Cant be called isinstance or typeof will run into infinite loop
# See qi.__init__ for the renaming
def _isinstance(a, type):
    """ return true if `a` is of type `type`
        .. warning::
           this function is only implemented for Object
    """
    if type != Object:
        raise NotImplementedError("isinstance is only implemented for Object right now")
    try:
        return typeof(a) == type
    except NotImplementedError:
        return False
