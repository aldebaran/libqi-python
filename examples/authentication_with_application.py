import argparse
import qi
import sys


class Authenticator:

    def __init__(self, username, password):
        self.username = username
        self.password = password

    # This method is expected by libqi and must return a dictionary containing
    # login information with the keys 'user' and 'token'.
    def initialAuthData(self):
        return {'user': self.username, 'token': self.password}


class AuthenticatorFactory:

    def __init__(self, username, password):
        self.username = username
        self.password = password

    # This method is expected by libqi and must return an object with at least
    # the `initialAuthData` method.
    def newAuthenticator(self):
        return Authenticator(self.username, self.password)


# Reads a file containing the username on the first line and the password on
# the second line.
def read_auth_file(path):
    with open(path) as f:
        username = f.readline().strip()
        password = f.readline().strip()
        return (username, password)


def make_application():
    app = qi.Application(sys.argv)
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a", "--authfile",
        help="Path to the authentication config file. This file must "
        "contain the username on the first line and the password on the "
        "second line.")
    args = parser.parse_args(sys.argv[1:])
    logins = read_auth_file(args.authfile) if args.authfile else ("nao", "nao")
    factory = AuthenticatorFactory(*logins)
    app.session.setClientAuthenticatorFactory(factory)
    return app


if __name__ == "__main__":
    app = make_application()
    logger = qi.Logger("authentication_with_application")
    logger.info("connecting session")
    app.start()
    logger.info("fetching ALTextToSpeech service")
    tts = app.session.service("ALTextToSpeech")
    logger.info("Saying something")
    tts.call("say", "Hello python")
