import argparse
import sys
import qi
import inspect

class Authenticator:

    def __init__(self, user, pswd):
        self.user = user
        self.pswd = pswd

    def initialAuthData(self):
        cm = {'user': self.user, 'token': self.pswd}
        return cm

class ClientFactory:

    def __init__(self, user, pswd):
        self.user = user
        self.pswd = pswd

    def newAuthenticator(self):
        return Authenticator(self.user, self.pswd)

def parse_options(argv=sys.argv[1:]):
    parser = argparse.ArgumentParser()
    parser.add_argument("url",             help="robot url (protocol + ip + port), eg tcps://10.0.0.0:9503")
    parser.add_argument("user",            help="username")
    parser.add_argument("pswd", nargs='?', help="password")
    args = parser.parse_args(argv)
    return args.url, args.user, args.pswd

def run():
    url, user, pswd = parse_options()
    factory = ClientFactory(user, pswd)
    session = qi.Session()
    session.setClientAuthenticatorFactory(factory)
    session.connect(url)
    tts = session.service("ALTextToSpeech")
    tts.call("say", "Hello python")


if __name__ == u"__main__":
    run()
