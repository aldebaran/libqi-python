#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Build a Python SDK Version """
from __future__ import absolute_import
from __future__ import print_function

import os
import setuptools

PLATFORM = os.environ.get("QIPYTHON_BUILD_PLATFORM")
SYSTEM = "Operating System :: POSIX :: Linux"
if PLATFORM == "win-amd64":
    SYSTEM = "Operating System :: Microsoft :: Windows"
elif PLATFORM == "macosx-10.12-intel":
    SYSTEM = "Operating System :: MacOS :: MacOS X"

PATH_HERE = os.path.abspath(os.path.dirname(__file__))
KEYWORDS = "naoqi softbank nao pepper romeo robot"
DESCRIPTION = "LibQi Python for NAOqi"
LONG_DESCRIPTION = """LibQi Python bindings for NAOqi\n\n---\n\nUsed to control SoftBank Robotics NAO and Pepper"""

setuptools.setup(
    name="qi",
    version=os.environ.get("QIPYTHON_BUILD_VERSION", "2.9.0.999"),
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    keywords=KEYWORDS,
    url="http://doc.aldebaran.com",
    author="SoftBank Robotics",
    author_email="release@softbankrobotics.com",
    platforms=PLATFORM,
    python_requires=">=2.6, <3",
    packages=["qi"],
    package_dir={"qi": "qi"},
    package_data={
        "qi": [
            "robot/*.*",
            "linux/*.*",
            "win/*.*",
            "mac/*.*",
            "mac/python2.7/site-packages/*.*",
        ]
    },
    include_package_data=True,
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Embedded Systems",
        "Framework :: Robot Framework :: Tool",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        SYSTEM,
    ],
)
