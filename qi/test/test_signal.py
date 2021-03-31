#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

import time
import qi


class subscriber:
    def __init__(self):
        self.done = False

    def callback(self):
        print("callback")
        self.done = True

    def callback_42(self, nb):
        print("callback_42:", nb)
        if (nb == 42):
            self.done = True

    def callback_5args(self, a, b, c, d, e):
        print("callback_5args:", a, b, c, d, e)
        self.done = True

    def wait(self):
        while not self.done:
            time.sleep(0.1)


def test_signal():
    print("\nInit...")
    sub1 = subscriber()
    sub2 = subscriber()
    mysignal = qi.Signal()

    print("\nTest #1 : Multiple subscribers to signal")
    callback = sub1.callback
    callback2 = sub2.callback
    mysignal.connect(callback)
    signalid = mysignal.connect(callback2)

    mysignal()
    sub2.wait()
    sub1.wait()

    print("\nTest #2 : Disconnect only one")
    mysignal.disconnect(signalid)

    sub1.done = False
    sub2.done = False
    mysignal()
    sub1.wait()

    print("\nTest #3 : Disconnect All")
    mysignal.connect(callback2)
    assert mysignal.disconnectAll()

    sub1.done = False
    sub2.done = False
    mysignal()
    time.sleep(0.5)

    assert not sub1.done
    assert not sub2.done

    print("\nTest #4 : Trigger with one parameter")
    mysignal.connect(sub1.callback_42)

    sub1.done = False
    sub2.done = False
    mysignal(42)
    sub1.wait()

    assert sub1.done
    assert not sub2.done

    assert mysignal.disconnectAll()
    print("\nTest #5 : Trigger with five parameters")
    mysignal.connect(sub1.callback_5args)

    sub1.done = False
    sub2.done = False
    mysignal(42, "hey", "a", 0.42, (0, 1))
    sub1.wait()
    assert sub1.done
    assert not sub2.done


lastSubs = False


def test_subscribe():
    global lastSubs
    lastSubs = False

    def onSubscriber(subs):
        global lastSubs
        lastSubs = subs
    sig = qi.Signal('m', onSubscriber)
    assert not lastSubs
    connlink = sig.connect(lambda x: None)
    assert lastSubs
    assert sig.disconnect(connlink)
    assert not lastSubs
