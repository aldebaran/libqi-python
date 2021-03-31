#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

from .qi_python import LogLevel, pylog, setLevel, setContext, setFilters
from collections import namedtuple
import inspect

__all__ = [
    "SILENT", "FATAL", "ERROR", "WARNING", "INFO", "VERBOSE", "DEBUG",
    "fatal", "error", "warning", "info", "verbose",
    "Logger", "setLevel", "setContext", "setFilters"
]

# Log Level
SILENT = LogLevel.Silent
FATAL = LogLevel.Fatal
ERROR = LogLevel.Error
WARNING = LogLevel.Warning
INFO = LogLevel.Info
VERBOSE = LogLevel.Verbose
DEBUG = LogLevel.Debug


def log_get_trace_info():
    info = None
    try:
        stack = inspect.stack()
        # current stack's frame 0 is this frame
        # frame 1 is the call on the Logger object
        # frame 2 must be the place where the call was made from
        callerframerecord = stack[2]
        frame = callerframerecord[0]
        info = inspect.getframeinfo(frame)
    except Exception:
        FakeTrackback = namedtuple("FakeTrackback",
                                   ["filename", "function", "lineno"])
        info = FakeTrackback('<file>', '<function>', -1)
    return info


def print_to_string(mess, *args):
    return ' '.join(str(x) for x in (mess,) + args)


class Logger:
    def __init__(self, category):
        self.category = category

    def fatal(self, mess, *args):
        """ fatal(mess, *args) -> None
        :param mess: Messages string
        :param *args: Messages format string working the same way as python
                      function print.
        Logs a message with level FATAL on this logger."""
        info = log_get_trace_info()
        pylog(FATAL, self.category, print_to_string(mess, *args),
              info.filename, info.function, info.lineno)

    def error(self, mess, *args):
        """ error(mess, *args) -> None
        :param mess: Messages string
        :param *args: Arguments are interpreted as for
                      :py:func:`qi.Logger.fatal`.
        Logs a message with level ERROR on this logger."""
        info = log_get_trace_info()
        pylog(ERROR, self.category, print_to_string(mess, *args),
              info.filename, info.function, info.lineno)

    def warning(self, mess, *args):
        """ warning(mess, *args) -> None
        :param mess: Messages string
        :param *args: Arguments are interpreted as for
                      :py:func:`qi.Logger.fatal`.
        Logs a message with level WARNING on this logger."""
        info = log_get_trace_info()
        pylog(WARNING, self.category, print_to_string(mess, *args),
              info.filename, info.function, info.lineno)

    def info(self, mess, *args):
        """ info(mess, *args) -> None
        :param mess: Messages string
        :param *args: Arguments are interpreted as for
                      :py:func:`qi.Logger.fatal`.
        Logs a message with level INFO on this logger."""
        info = log_get_trace_info()
        pylog(INFO, self.category, print_to_string(mess, *args),
              info.filename, info.function, info.lineno)

    def verbose(self, mess, *args):
        """ verbose(mess, *args) -> None
        :param mess: Messages string
        :param *args: Arguments are interpreted as for
                      :py:func:`qi.Logger.fatal`.
        Logs a message with level VERBOSE on this logger."""
        info = log_get_trace_info()
        pylog(VERBOSE, self.category, print_to_string(mess, *args),
              info.filename, info.function, info.lineno)


def fatal(cat, mess, *args):
    """ fatal(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical
                value.
    :param mess: Messages string
    :param *args: Messages format string working the same way as print python
                  function.
    Logs a message with level FATAL."""
    info = log_get_trace_info()
    pylog(FATAL, cat, print_to_string(mess, *args),
          info.filename, info.function, info.lineno)


def error(cat, mess, *args):
    """ error(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical
                value.
    :param mess: Messages string
    :param *args: Messages format string working the same way as print python
                  function.
    Logs a message with level ERROR."""
    info = log_get_trace_info()
    pylog(ERROR, cat, print_to_string(mess, *args),
          info.filename, info.function, info.lineno)


def warning(cat, mess, *args):
    """ warning(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical
                value.
    :param mess: Messages string
    :param *args: Messages format string working the same way as print python
                  function.
    Logs a message with level WARNING."""
    info = log_get_trace_info()
    pylog(WARNING, cat, print_to_string(mess, *args),
          info.filename, info.function, info.lineno)


def info(cat, mess, *args):
    """ info(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical
                value.
    :param mess: Messages string
    :param *args: Messages format string working the same way as print python
                  function.
    Logs a message with level INFO."""
    info = log_get_trace_info()
    pylog(INFO, cat, print_to_string(mess, *args),
          info.filename, info.function, info.lineno)


def verbose(cat, mess, *args):
    """ verbose(cat, mess, *args) -> None
    :param cat: The category is potentially a period-separated hierarchical
                value.
    :param mess: Messages string
    :param *args: Messages format string working the same way as print python
                  function.
    Logs a message with level VERBOSE."""
    info = log_get_trace_info()
    pylog(VERBOSE, cat, print_to_string(mess, *args),
          info.filename, info.function, info.lineno)
