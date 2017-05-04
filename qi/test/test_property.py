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

@pytest.fixture()
def property_service_fixture()
    def run_property_service_fixture(signature)
        class PropObj:
            def __init__(self):
                self.prop = qi.Property(signature)

        class PropServiceFixture:
            def __init__(self, servSess, servObj, clientSess, clientObj):
              self.servSess = servSess
              self.servObj = servObj
              self.clientSess = clientSess
              self.clientObj = clientObj

            def assert_both_sides_eq(val):
              if val is None:
                assert self.servObj.prop.value() is None
                assert self.clientObj.prop.value() is None
              else:
                assert self.servObj.prop.value() == val
                assert self.clientObj.prop.value() == val

        servSess = qi.Session()
        servSess.listenStandalone('tcp://localhost:0')
        servObj = PropObj()
        servSess.registerService('Serv', servObj)

        clientSess = qi.Session()
        clientSess.connect(s1.url())
        clientObj = s2.service('Serv')
        return PropServiceFixture(servSess, servObj, clientSess, clientObj)
  return run_property_service

def test_remote_property(property_service_fixture):
    fixture = property_service_fixture('s')
    servObj, clientObj = fixture.servObj, fixture.clientObj

    servObj.prop.setValue("rofl")
    fixture.assert_both_sides_eq("rofl")

    clientObj.prop.setValue("lol")
    fixture.assert_both_sides_eq("lol")

    test_remote_property.gotval = ""
    def cb(val):
        test_remote_property.gotval += val
    rlink = clientObj.prop.addCallback(cb)
    llink = servObj.prop.addCallback(cb)
    servObj.prop.setValue("ha")
    clientObj.prop.setValue("ha")
    time.sleep(0.05)
    assert test_remote_property.gotval == "hahahaha"
    clientObj.prop.disconnect(rlink)
    servObj.prop.disconnect(llink)
    servObj.prop.setValue("ha")
    clientObj.prop.setValue("ha")
    time.sleep(0.05)
    assert test_remote_property.gotval == "hahahaha"

def test_optional_int():
    prop = qi.Property('+i')
    assert prop.value() is None
    prop.setValue(42)
    assert prop.value() == 42
    prop.setValue(None)
    assert prop.value() is None

def test_non_optional_cannot_be_none():
    prop = qi.Property('i')
    assert prop.value() is not None
    with pytest.raises(RuntimeError):
      prop.setValue(None)

def test_remote_optional_property(property_service_fixture):
    fixture = property_service_fixture('+s')
    servObj, clientObj = fixture.servObj, fixture.clientObj

    fixture.assert_both_sides_eq(None)

    servObj.prop.setValue("cookies")
    fixture.assert_both_sides_eq("cookies")

    servObj.prop.setValue(None)
    fixture.assert_both_sides_eq(None)

    clientObj.prop.setValue("muffins")
    fixture.assert_both_sides_eq("muffins")

    clientObj.prop.setValue(None)
    fixture.assert_both_sides_eq(None)
