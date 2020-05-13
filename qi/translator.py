#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" QiMessaging Python bindings """
from __future__ import absolute_import
# from __future__ import unicode_literals

from _qi import Translator
from .logging import warning

globTranslator = None

def defaultTranslator(name):
    global globTranslator
    if globTranslator:
        return globTranslator
    globTranslator = Translator(name)
    return globTranslator

def tr(msg, domain=None, locale=None):
    global globTranslator
    if not globTranslator:
        warning("qi.translator", "You must init your translator first.")
        return msg
    if domain is None:
        return globTranslator.translate(msg)
    if locale is None:
        return globTranslator.translate(msg, domain)
    return globTranslator.translate(msg, domain, locale)

__all__ = ("defaultTranslator", "tr")
