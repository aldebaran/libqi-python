#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Upload  """
from __future__ import absolute_import
from __future__ import unicode_literals
from __future__ import print_function

import os

from twine.commands.upload import main as twine_upload

CURRENT_PATH = os.path.dirname(os.path.realpath(__file__))

if __name__ == "__main__":
    if os.environ.get("TWINE_USERNAME") is None:
        os.environ["TWINE_USERNAME"] = "sbr"
        print("No TWINE_USERNAME, using sbr")
    if os.environ.get("TWINE_PASSWORD") is None:
        print("No TWINE_PASSWORD, upload cancelled")
        exit()
    dist_dir = os.path.join(CURRENT_PATH, "dist")
    if not os.listdir(dist_dir):
        print("Nothing to upload")
        exit()
    twine_upload([os.path.join(dist_dir, "*")])
