##
## Author(s):
##  - Herve CUCHE <hcuche@aldebaran-robotics.com>
##
## Copyright (C) 2013 Aldebaran Robotics
##

from _qi import pylog
from _qi import setLevel, setContext, setFilters
import inspect
import uuid

__all__ = ("SILENT", "FATAL", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG",
           "fatal", "error", "warning", "info", "verbose",
           "Logger", "setLevel", "setContext", "setFilters",
           "getLogger", "logFatal", "logError", "logWarning", "logInfo", "logVerbose", "logDebug",  #deprecated
           )

# Log Level
SILENT  = 0
FATAL   = 1
ERROR   = 2
WARNING = 3
INFO    = 4
VERBOSE = 5
DEBUG   = 6

def _logGetTraceInfo():
    info = None
    try:
        stack = inspect.stack()

        # current stack's frame 0 is this frame
        # frame 1 is the call on the Logger object
        # frame 2 must be the place where the call was made from
        callerframerecord = stack[2]

        frame = callerframerecord[0]
        info = inspect.getframeinfo(frame)

    except:
        info = Object()
        info.filename = '<file>'
        info.function = '<function>'
        info.lineno = -1
    return info

def _printToString(mess, *args):
    margs = (mess,) + args
    return ' '.join(str(x) for x in margs)


class Logger:
    def __init__(self, category):
        self.category = category

    def fatal(self, mess, *args):
        """ fatal(mess, *args) -> None
        :param mess: Messages string
        :param \*args: Messages format string working the same way as python function print.

        Logs a message with level FATAL on this logger."""
        info = _logGetTraceInfo()
        pylog(FATAL, self.category, _printToString(mess, *args), info.filename, info.function, info.lineno)

    def error(self, mess, *args):
        """ error(mess, *args) -> None
        :param mess: Messages string
        :param \*args: Arguments are interpreted as for :py:func:`qi.Logger.fatal`.

        Logs a message with level ERROR on this logger."""
        info = _logGetTraceInfo()
        pylog(ERROR, self.category, _printToString(mess, *args), info.filename, info.function, info.lineno)

    def warning(self, mess, *args):
        """ warning(mess, *args) -> None
        :param mess: Messages string
        :param \*args: Arguments are interpreted as for :py:func:`qi.Logger.fatal`.

        Logs a message with level WARNING on this logger."""
        info = _logGetTraceInfo()
        pylog(WARNING, self.category, _printToString(mess, *args), info.filename, info.function, info.lineno)

    def info(self, mess, *args):
        """ info(mess, *args) -> None
        :param mess: Messages string
        :param \*args: Arguments are interpreted as for :py:func:`qi.Logger.fatal`.

        Logs a message with level INFO on this logger."""
        info = _logGetTraceInfo()
        pylog(INFO, self.category, _printToString(mess, *args), info.filename, info.function, info.lineno)

    def verbose(self, mess, *args):
        """ verbose(mess, *args) -> None
        :param mess: Messages string
        :param \*args: Arguments are interpreted as for :py:func:`qi.Logger.fatal`.

        Logs a message with level VERBOSE on this logger."""
        info = _logGetTraceInfo()
        pylog(VERBOSE, self.category, _printToString(mess, *args), info.filename, info.function, info.lineno)

def fatal(cat, mess, *args):
    """ fatal(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical value.
    :param mess: Messages string
    :param \*args: Messages format string working the same way as print python function.

    Logs a message with level FATAL."""
    info = _logGetTraceInfo()
    pylog(FATAL, cat, _printToString(mess, *args), info.filename, info.function, info.lineno)

def error(cat, mess, *args):
    """ error(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical value.
    :param mess: Messages string
    :param \*args: Messages format string working the same way as print python function.

    Logs a message with level ERROR."""
    info = _logGetTraceInfo()
    pylog(ERROR, cat, _printToString(mess, *args), info.filename, info.function, info.lineno)

def warning(cat, mess, *args):
    """ warning(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical value.
    :param mess: Messages string
    :param \*args: Messages format string working the same way as print python function.

    Logs a message with level WARNING."""
    info = _logGetTraceInfo()
    pylog(WARNING, cat, _printToString(mess, *args), info.filename, info.function, info.lineno)

def info(cat, mess, *args):
    """ info(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical value.
    :param mess: Messages string
    :param \*args: Messages format string working the same way as print python function.

    Logs a message with level INFO."""
    info = _logGetTraceInfo()
    pylog(INFO, cat, _printToString(mess, *args), info.filename, info.function, info.lineno)

def verbose(cat, mess, *args):
    """ verbose(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical value.
    :param mess: Messages string
    :param \*args: Messages format string working the same way as print python function.

    Logs a message with level VERBOSE."""
    info = _logGetTraceInfo()
    pylog(VERBOSE, cat, _printToString(mess, *args), info.filename, info.function, info.lineno)


#deprecated 2.0.1  (to remove in 2.1)
logFatal   = fatal
logError   = error
logWarning = warning
logInfo    = info
logVerbose = verbose
logDebug   = verbose
getLogger  = Logger
