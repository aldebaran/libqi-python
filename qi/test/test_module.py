import qi
import sys
import pytest

def test_module():
    mod = qi.module("moduletest")

    cat = mod.createObject("Cat", "truc")
    assert cat.meow(3) == 'meow'

    mouse = mod.createObject("Mouse")
    assert mouse.squeak() == 18

    assert mod.call("lol") == 3
