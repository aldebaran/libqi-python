import time
import qi

isconnected = False
isdisconnected = False

def test_session_make_does_not_crash():
    session = qi.Session()

def test_session_listen_then_close_does_not_crash_or_deadlock():
    session = qi.Session()
    session.listenStandalone('tcp://127.0.0.1:0')
    session.close()

def test_session_callbacks():
    def callback_conn():
        global isconnected
        isconnected = True
        print("Connected!")

    def callback_disconn(s):
        global isdisconnected
        isdisconnected = True
        print("Disconnected!")

    local = "tcp://127.0.0.1:0"
    sd = qi.Session()
    sd.listenStandalone(local)

    s = qi.Session()
    assert not s.isConnected()
    assert not isconnected
    s.connected.connect(callback_conn)
    s.disconnected.connect(callback_disconn)
    s.connect(sd.endpoints()[0])
    time.sleep(0.01)

    assert s.isConnected()
    assert isconnected
    assert not isdisconnected

    s.close()
    time.sleep(0.01)

    assert isdisconnected

def main():
    test_session_callbacks()

if __name__ == "__main__":
    main()
