#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

import time
import threading
import pytest
from qi import Promise, Future, futureBarrier, runAsync

pytest_plugins = ("timeout",)


def waiterSetValue(promise, waiter):
    # time.sleep(t)
    waiter.wait()
    try:
        promise.setValue("mjolk")
    except Exception:
        pass


def waitSetValue(p, t=0.01):
    time.sleep(t)
    p.setValue("mjolk")


def test_many_futures_create():
    def wait(p):
        time.sleep(1)
    p = Promise(wait)
    fs = [p.future() for _ in range(100)]
    p.setValue(1337)
    for f in fs:
        assert f.hasValue()
        assert f.value() == 1337


def test_future_wait():
    p = Promise()
    f = p.future()
    threading.Thread(target=waitSetValue, args=[p, 0.1]).start()
    assert f.isFinished() is False
    assert f.value() == "mjolk"
    assert f.isFinished() is True


def test_many_futures_wait_cancel():
    def cancel(p):
        try:
            p.setValue("Kappa")
        except Exception:
            pass  # ok: cancel called many times

    waiter = Promise()
    ps = [Promise(cancel) for _ in range(50)]
    fs = [p.future() for p in ps]
    for p in ps:
        threading.Thread(target=waiterSetValue, args=[
                         p, waiter.future()]).start()
    # Cancel only one future
    fs[25].cancel()
    waiter.setValue(None)

    for i, f in enumerate(fs):
        if i == 25:
            assert f.value() == "Kappa"
        else:
            assert f.value() == "mjolk"


def test_many_promises_wait_cancel():
    def cancel(p):
        try:
            p.setValue("Kappa")
        except Exception:
            pass  # ok: cancel called many times

    waiter = Promise()
    ps = [Promise(cancel) for _ in range(50)]
    fs = [p.future() for p in ps]
    for p in ps:
        threading.Thread(target=waiterSetValue, args=[
                         p, waiter.future()]).start()
    # Cancel only one promise
    ps[25].setCanceled()
    waiter.setValue(None)
    for i, f in enumerate(fs):
        if i == 25:
            try:
                f.value()
            except RuntimeError:
                pass
        else:
            assert f.value() == "mjolk"


def test_future_cancel_request():
    promise = Promise()
    future = promise.future()
    future.cancel()
    assert promise.isCancelRequested()


def test_future_no_timeout():
    p = Promise()
    f = p.future()
    threading.Thread(target=waitSetValue, args=[p]).start()
    # 1sec to be secure
    assert f.value(timeout=1000) == "mjolk"
    assert f.hasError() is False


def test_future_timeout_immediate():
    p = Promise()
    f = p.future()
    threading.Thread(target=waitSetValue, args=[p, 1]).start()
    try:
        f.value(timeout=0)
    except RuntimeError:
        pass


def test_future_timeout():
    p = Promise()
    f = p.future()
    threading.Thread(target=waitSetValue, args=[p, 1]).start()
    try:
        # 10ms - 3ms
        f.value(timeout=8)
    except RuntimeError:
        pass
    assert f.hasError() is False


def test_future_error():
    p = Promise()
    p.setError("woops")
    f = p.future()
    assert f.hasError() is True
    assert f.error() == "woops"
    try:
        f.value()
    except RuntimeError:
        pass


def test_future_cancel_exception():
    def throw(promise):
        time.sleep(0.01)
        raise Exception("plop")

    p = Promise(throw)
    f = p.future()
    f.cancel()


called = False


def test_future_callback():

    result = Promise()

    def callback(f):
        global called
        assert f.isRunning() is False
        assert f.value() == 1337
        assert called is False
        called = "aight"
        result.setValue("bim")

    p = Promise()
    f = p.future()
    f.addCallback(callback)
    p.setValue(1337)
    result.future().wait(1000)
    assert result.future().hasValue(0)
    assert called == "aight"
    assert not f.isCanceled()
    assert f.isFinished()


def test_future_then():

    def callback(f):
        assert f.isRunning() is False
        assert f.value() == 1337
        return 4242

    p = Promise()
    f = p.future()
    f2 = f.then(callback)
    p.setValue(1337)
    f2.wait(1000)
    assert f2.isFinished()
    assert f2.value() == 4242


def test_future_then_throw():

    def callback(f):
        assert f.isRunning() is False
        assert f.value() == 1337
        raise RuntimeError("lol")

    p = Promise()
    f = p.future()
    f2 = f.then(callback)
    p.setValue(1337)
    f2.wait(1000)
    assert f2.isFinished()
    assert "RuntimeError: lol" in f2.error()


def test_future_andthen():

    def callback(v):
        assert v == 1337
        return 4242

    p = Promise()
    f = p.future()
    f2 = f.andThen(callback)
    p.setValue(1337)
    f2.wait(1000)
    assert f2.isFinished()
    assert f2.value() == 4242


def test_future_andthen_error():
    global called
    called = False

    def callback(v):
        global called
        called = True
        assert False

    p = Promise()
    f = p.future()
    f2 = f.andThen(callback)
    p.setError("errlol")
    f2.wait(1000)
    assert f2.isFinished()
    assert f2.error() == "errlol"
    time.sleep(0.1)
    assert not called


def test_future_andthen_cancel():
    global called
    called = False

    def callback(v):
        global called
        called = True
        assert False

    p = Promise()
    f = p.future()
    f2 = f.andThen(callback)
    f2.cancel()
    assert p.isCancelRequested()
    p.setCanceled()
    f2.wait(1000)
    assert f2.isFinished()
    assert f2.isCanceled()
    time.sleep(0.1)
    assert not called


called1, called2 = "", ""


def test_future_two_callbacks():

    result1 = Promise()
    result2 = Promise()

    def callback1(f):
        global called1
        called1 = "1"
        result1.setValue("bim")

    def callback2(f):
        global called2
        called2 = "2"
        result2.setValue("bim")

    p = Promise()
    f = p.future()
    f.addCallback(callback1)
    f.addCallback(callback2)
    p.setValue(42)

    result1.future().wait(1000)
    result2.future().wait(1000)

    assert result1.future().hasValue(0)
    assert result2.future().hasValue(0)
    assert called1 == "1"
    assert called2 == "2"


def test_future_callback_noargs():
    def callback():
        pass
    p = Promise()
    f = p.future()
    f.addCallback(callback)
    p.setValue("segv?")
    assert not f.isCanceled()
    assert f.isFinished()


def test_promise_re_set():
    p = Promise()
    p.setValue(42)
    try:
        p.setValue(42)
    except RuntimeError:
        pass


def test_future_exception():
    p = Promise()
    f = p.future()

    def raising(f):
        raise Exception("oops")

    f.addCallback(raising)
    p.setValue(42)
    # This test doesn't assert, it's only segfault check
    # No segfault => no bug


def make_future_callback(nbr_fut):
    def callback(f):
        pass

    for _ in range(nbr_fut):
        p = Promise()
        f = p.future()
        f.addCallback(callback)
        p.setValue(0)


def test_future_many_callback():
  make_future_callback(10000)


def test_many_callback_threaded():
    nbr_threads = 100
    thr_list = []
    for i in range(nbr_threads):
        thr = threading.Thread(
            target=make_future_callback, kwargs={"nbr_fut": 10})
        thr_list.append(thr)

    for thr in thr_list:
        thr.start()

    for thr in thr_list:
        try:
          thr.join()
        except Exception as ex:
          pytest.fail(str(ex))


def test_future_init():
    fut = Future(30)
    assert fut.value() == 30


def test_future_unwrap():
    prom = Promise()
    future = prom.future().unwrap()
    prom.setValue(Future(42))

    assert future.value() == 42


def test_future_unwrap_cancel():
    prom = Promise()
    future = prom.future().unwrap()
    future.cancel()

    assert prom.isCancelRequested()
    nested = Promise()
    prom.setValue(nested.future())
    # TODO add some sync-ness to remove those time.sleep :(
    time.sleep(0.1)
    assert nested.isCancelRequested()
    nested.setCanceled()
    time.sleep(0.1)
    assert future.isCanceled()


def test_future_unwrap_notfuture():
    prom = Promise()
    future = prom.future().unwrap()
    prom.setValue(42)

    assert future.hasError()


def test_future_barrier():
    proms = [Promise() for x in range(2)]

    f = futureBarrier([p.future() for p in proms])
    for p in proms:
        p.setValue(0)
    futs = f.value()
    for fut in futs:
        assert fut.value() == 0


def test_future_barrier_cancel():
    proms = [Promise() for x in range(2)]

    f = futureBarrier([p.future() for p in proms])
    proms[0].setValue(0)
    f.cancel()
    assert proms[1].isCancelRequested()
    proms[1].setCanceled()
    futs = f.value()
    assert futs[0].value() == 0
    assert futs[1].isCanceled()


@pytest.mark.timeout(5)
def test_promise_concurrent_setvalue_setcanceled():
    delay_in_sec = 0.01
    delay_in_microsec = int(1e6 * delay_in_sec)
    for _ in range(100):
        promise = Promise()
        # this seemingly does nothing but greatly increases chances of lock:
        promise.future().addCallback(lambda fut: None)
        runAsync(promise.setValue, None, delay=delay_in_microsec)
        time.sleep(delay_in_sec)
        try:
            promise.setCanceled()
        except RuntimeError:
            pass
