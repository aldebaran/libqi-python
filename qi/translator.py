#
# Copyright (C) 2010 - 2020 Softbank Robotics Europe
#
# -*- coding: utf-8 -*-

from .qi_python import Translator
from .logging import warning

__all__ = ("defaultTranslator", "tr")

glob_translator = None


def defaultTranslator(name):
    global glob_translator
    if glob_translator:
        return glob_translator
    glob_translator = Translator(name)
    return glob_translator


def tr(msg, domain=None, locale=None):
    global glob_translator
    if not glob_translator:
        warning("qi.translator", "You must init your translator first.")
        return msg
    if domain is None:
        return glob_translator.translate(msg)
    if locale is None:
        return glob_translator.translate(msg, domain)
    return glob_translator.translate(msg, domain, locale)
