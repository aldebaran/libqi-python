import qi
import sys
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

    assert mod.call("lol") == 3
