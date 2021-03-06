#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

import qi
import time


def add(a, b, c = 1):
    return (a + b) * c


def fail():
    assert(False)


def err():
    raise RuntimeError("sdsd")


def test_async_fun():
    f = qi.runAsync(add, 21, 21)
    assert(f.value() == 42)

def test_async_fun_kw():
    # delay is a kw arg for runAsync, the others are kw args for add.
    f = qi.runAsync(add, c=3, b=9, a=4, delay=100)
    assert(f.value() == 39)


def test_async_error():
    f = qi.runAsync(err)
    assert(f.hasError())
    assert("RuntimeError: sdsd" in f.error())


class Adder:
    def __init__(self):
        self.v = 0

    def add(self, a):
        self.v += a
        return self.v

    def val(self):
        return self.v


def test_async_meth():
    ad = Adder()
    f = qi.runAsync(ad.add, 21)
    assert(f.value() == 21)
    f = qi.runAsync(ad.add, 21)
    assert(f.value() == 42)
    f = qi.runAsync(ad.val)
    assert(f.value() == 42)


def test_async_delay():
    f = qi.runAsync(add, 21,  21, delay=1000)
    assert(f.value() == 42)

def test_periodic_task():
    t = qi.PeriodicTask()
    global result
    result = 0
    def add():
        global result
        result += 1
    t.setCallback(add)
    t.setUsPeriod(1000)
    t.start(True)
    time.sleep(1)
    t.stop()
    assert result > 5  # how to find 5: plouf plouf plouf
    cur = result
    time.sleep(1)
    assert cur == result
    del result


def test_async_cancel():
    f = qi.runAsync(fail, delay=1000000)
    assert(f.isCancelable())
    f.cancel()
    f.wait()
    assert(f.isFinished())
    assert(not f.hasError())
    assert(f.isCanceled())


def test_async_nested_future():
    f = qi.runAsync(lambda: qi.Future(42))
    assert isinstance(f, qi.Future)
    assert isinstance(f.value(), qi.Future)
