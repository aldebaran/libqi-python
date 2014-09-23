#!/usr/bin/env python2
##
## Author(s):
##  - Philippe Daouadi <pdaouadi@aldebaran.com>
##
## Copyright (C) 2013 Aldebaran Robotics

import qi
import time
import threading
import pytest

def test_set():
    prop = qi.Property('i')
    prop.setValue(15)
    assert prop.value() == 15
    with pytest.raises(RuntimeError):
        prop.setValue("lol")

def test_signal():
    prop = qi.Property('i')
    test_signal.gotval = 0
    def cb(val):
        test_signal.gotval += val
    link = prop.connect(cb)
    prop.setValue(42)
    time.sleep(0.05)
    assert test_signal.gotval == 42
    prop.disconnect(link)
    prop.setValue(38)
    time.sleep(0.05)
    assert test_signal.gotval == 42

class PropObj:
    def __init__(self):
        self.prop = qi.Property('s')

def test_remote_property():
    s1 = qi.Session()
    s1.listenStandalone('tcp://localhost:9999')
    lobj = PropObj()
    s1.registerService('Serv', lobj)

    s2 = qi.Session()
    s2.connect(s1.url())
    robj = s2.service('Serv')
    lobj.prop.setValue("rofl")
    assert lobj.prop.value() == "rofl"
    assert robj.prop.value() == "rofl"
    robj.prop.setValue("lol")
    assert robj.prop.value() == "lol"
    assert lobj.prop.value() == "lol"

    test_remote_property.gotval = ""
    def cb(val):
        test_remote_property.gotval += val
    rlink = robj.prop.addCallback(cb)
    llink = lobj.prop.addCallback(cb)
    lobj.prop.setValue("ha")
    robj.prop.setValue("ha")
    time.sleep(0.05)
    assert test_remote_property.gotval == "hahahaha"
    robj.prop.disconnect(rlink)
    lobj.prop.disconnect(llink)
    lobj.prop.setValue("ha")
    robj.prop.setValue("ha")
    time.sleep(0.05)
    assert test_remote_property.gotval == "hahahaha"
