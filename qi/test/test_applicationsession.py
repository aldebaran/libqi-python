import time
import sys
import qi

global usual_timeout
usual_timeout = 10

def has_succeeded(promise):
    return promise.future().wait(usual_timeout) == qi.FutureState.FinishedWithValue

def test_applicationsession():

    connecting = qi.Promise()

    def callback_conn():
        connecting.setValue(None)
        print("Connected!")

    def has_connected():
        return has_succeeded(connecting)


    disconnecting = qi.Promise()

    def callback_disconn(s):
        disconnecting.setValue(None)
        print("Disconnected!")

    def has_disconnected():
        return has_succeeded(disconnecting)


    sd = qi.Session()
    sd.listenStandalone("tcp://127.0.0.1:0")

    sys.argv = sys.argv + ["--qi-url", sd.endpoints()[0]]
    app = qi.ApplicationSession(sys.argv)
    assert app.url == sd.endpoints()[0]
    assert not has_connected()
    assert not has_disconnected()
    app.session.connected.connect(callback_conn)
    app.session.disconnected.connect(callback_disconn)
    app.start()

    assert has_connected()
    assert not has_disconnected()

    running = qi.Promise()
    def validate():
        running.setValue(None)

    def has_run():
        return has_succeeded(running)

    app.atRun(validate)

    def runApp():
        app.run()

    qi.async(runApp)
    assert has_run()

    app.session.close()
    time.sleep(0.01)

    assert has_disconnected()

def main():
    test_applicationsession()

if __name__ == "__main__":
    main()
