#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

import qi
import pytest


def test_module():
    mod = qi.module("moduletest")

    cat = mod.createObject("Cat", "truc")
    assert cat.meow(3) == 'meow'

    mouse = mod.createObject("Mouse")
    assert mouse.squeak() == 18

    session = qi.Session()
    session.listenStandalone('tcp://localhost:0')
    cat = mod.createObject("Cat", session)
    assert cat.meow(3) == 'meow'

    assert cat.cloneMe().meow(3) == 'meow'

    assert mod.call("lol") == 3


def test_module_undef():
    mod = qi.module("moduletest")

    with pytest.raises(AttributeError):
        mod.createObject("LOL")


def test_module_service():
    session = qi.Session()
    session.listenStandalone("tcp://localhost:0")

    session.loadServiceRename("moduletest.Cat", "", "truc")

    cat = session.service("Cat")
    assert cat.meow(3) == 'meow'

def test_module_service_object_lifetime():
    session = qi.Session()
    session.listenStandalone("tcp://localhost:0")
    session.loadServiceRename("moduletest.Cat", "", "truc")
    cat = session.service("Cat")

    # We use `del` to release the reference to an object, but there is no
    # guarantee that the interpreter will finalize the object right away.
    # In CPython, this is "mostly" guaranteed as long as it is the last
    # reference to the object and that there is no cyclic dependency of
    # reference anywhere that might hold the reference.
    # As there is no practical alternative, we consider that assuming the
    # object is finalized immediately is an acceptable hypothesis.

    # Purr has a property, Play has a signal, Sleep has none.
    sleep = cat.makeSleep()
    assert cat.nbSleep() == 1
    del sleep
    assert cat.nbSleep() == 0

    purr = cat.makePurr()
    assert cat.nbPurr() == 1
    del purr
    assert cat.nbPurr() == 0

    play = cat.makePlay()
    assert cat.nbPlay() == 1
    del play
    assert cat.nbPlay() == 0

def test_object_bound_functions_arguments_conversion_does_not_leak():
    session = qi.Session()
    session.listenStandalone("tcp://localhost:0")
    session.loadServiceRename("moduletest.Cat", "", "truc")
    cat = session.service("Cat")

    play = cat.makePlay()
    assert cat.nbPlay() == 1
    cat.order(play)
    assert cat.nbPlay() == 1
    del play
    assert cat.nbPlay() == 0

def test_temporary_object_bound_properties_are_usable():
    session = qi.Session()
    session.listenStandalone("tcp://localhost:0")
    session.loadServiceRename("moduletest.Cat", "", "truc")
    # The `volume` member is a bound property of the `Purr` object.
    # It should keep the object alive so that setting the property, which
    # requires accessing the object, does not fail.
    session.service("Cat").makePurr().volume.setValue(42)
