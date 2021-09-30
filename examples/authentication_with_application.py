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
# the second line. This is the format used by qilaunch.
def read_auth_file(path):
    with open(path) as f:
        username = f.readline().strip()
        password = f.readline().strip()
        return (username, password)


def make_application(argv=sys.argv):
    """
    Create and return the qi.Application, with authentication set up
    according to the command line options.
    """
    # create the app and edit `argv` in place to remove the consumed
    # arguments.
    # As a side effect, if "-h" is in the list, it is replaced with "--help".
    app = qi.Application(argv)

    # Setup a non-intrusive parser, behaving like `qi.Application`'s own
    # parser:
    # * don't complain about unknown arguments
    # * consume known arguments
    # * if the "--help" option is present:
    #   * print its own options help
    #   * do not print the main app usage
    #   * do not call `sys.exit()`
    parser = argparse.ArgumentParser(add_help=False, usage=argparse.SUPPRESS)
    parser.add_argument(
        "-a", "--authfile",
        help="Path to the authentication config file. This file must "
        "contain the username on the first line and the password on the "
        "second line.")
    if "--help" in argv:
        parser.print_help()
        return app
    args, unparsed_args = parser.parse_known_args(argv[1:])
    logins = read_auth_file(args.authfile) if args.authfile else ("nao", "nao")
    factory = AuthenticatorFactory(*logins)
    app.session.setClientAuthenticatorFactory(factory)
    # edit argv in place.
    # Note: this might modify sys.argv, like qi.Application does.
    argv[1:] = unparsed_args
    return app


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--msg", default="Hello python")
    app = make_application()
    args = parser.parse_args()
    logger = qi.Logger("authentication_with_application")
    logger.info("connecting session")
    app.start()
    logger.info("fetching ALTextToSpeech service")
    tts = app.session.service("ALTextToSpeech")
    logger.info("Saying something")
    tts.call("say", args.msg)
