import qi
import time
from functools import partial

@qi.singleThreaded()
class Stranded:
    def __init__(self, max):
        self.calling = False
        self.counter = 0
        self.max = max
        self.end = qi.Promise()

    def cb(self, val):
        assert not self.calling
        self.calling = True

        time.sleep(0.01)

        assert self.calling
        self.calling = False

        self.counter += 1

        if self.counter == self.max:
            self.end.setValue(None)

    def cbn(self, *args):
        self.cb(args[-1])

def test_future_strand():
    obj = Stranded(30)

    prom = qi.Promise()
    for i in range(30):
        prom.future().addCallback(obj.cb)
    prom.setValue(None)

    obj.end.future().value()

def test_future_partials_strand():
    obj = Stranded(50)

    prom = qi.Promise()
    for i in range(10):
        prom.future().addCallback(obj.cb)
    for i in range(10):
        prom.future().addCallback(partial(obj.cbn, 1))
    for i in range(10):
        prom.future().addCallback(partial(partial(obj.cbn, 1), 2))
    for i in range(10):
        prom.future().addCallback(partial(partial(Stranded.cbn, obj, 1), 2))
    for i in range(10):
        prom.future().addCallback(partial(Stranded.cbn, obj, 1))
    prom.setValue(None)

    obj.end.future().value()

def test_signal_strand():
    obj = Stranded(30)

    sig = qi.Signal("m")
    for i in range(30):
        sig.connect(obj.cb)
    sig(None)

    obj.end.future().value()

def test_remote_signal_strand():
    server = qi.Session()
    server.listenStandalone("tcp://localhost:0")
    client = qi.Session()
    client.connect(server.url())

    obj = Stranded(30)

    class Serv:
        def __init__(self):
            self.sig = qi.Signal('m')
    s = Serv()
    server.registerService('Serv', s)

    sig = client.service('Serv').sig
    for i in range(30):
        sig.connect(obj.cb)
    s.sig(None)

    obj.end.future().value()

def test_property_strand():
    obj = Stranded(30)

    prop = qi.Property("m")
    for i in range(30):
        prop.connect(obj.cb)
    prop.setValue(42)

    obj.end.future().value()

def test_remote_call_strand():
    server = qi.Session()
    server.listenStandalone("tcp://localhost:0")
    client = qi.Session()
    client.connect(server.url())

    obj = Stranded(50)

    server.registerService('Serv', obj)

    robj = client.service('Serv')
    for i in range(25):
        robj.cb(None, _async=True)
    for i in range(25):
        qi.async(partial(obj.cb, None))

    obj.end.future().value()

def test_remote_property_strand():
    server = qi.Session()
    server.listenStandalone("tcp://localhost:0")
    client = qi.Session()
    client.connect(server.url())

    obj = Stranded(30)

    class Serv:
        def __init__(self):
            self.prop = qi.Property('m')
    s = Serv()
    server.registerService('Serv', s)

    prop = client.service('Serv').prop
    for i in range(30):
        prop.connect(obj.cb)
    s.prop.setValue(42)

    obj.end.future().value()

def test_async_strand():
    obj = Stranded(30)
    for i in range(30):
        qi.async(partial(obj.cb, None))
    obj.end.future().value()

# periodic task is tested only here as there is no point in testing it alone
def test_all_strand():
    obj = Stranded(81)

    sig = qi.Signal("m")
    prop = qi.Property("m")
    prom = qi.Promise()
    per = qi.PeriodicTask()
    per.setCallback(partial(obj.cb, None))
    per.setUsPeriod(10000);
    for i in range(20):
        prom.future().addCallback(obj.cb)
    for i in range(20):
        sig.connect(obj.cb)
    for i in range(20):
        prop.connect(obj.cb)
    per.start(True);
    sig(None)
    prop.setValue(42)
    prom.setValue(None)
    for i in range(20):
        qi.async(partial(obj.cb, None))

    obj.end.future().value()
    per.stop();

if __name__ == '__main__':
    test_future_strand()
    test_future_partials_strand()
    test_signal_strand()
    test_remote_signal_strand()
    test_property_strand()
    test_remote_property_strand()
    test_async_strand()
    test_all_strand()
