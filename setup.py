#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import platform
import pathlib
from setuptools import find_packages
from skbuild import setup
from packaging import version
import toml

py_version = version.parse(platform.python_version())
min_version = version.parse('3.5')
if py_version < min_version:
    raise RuntimeError('Python 3.5+ is required.')

# Parse `pyproject.toml` runtime dependencies.
pyproject_data = toml.loads(pathlib.Path('pyproject.toml').read_text())
pyproject_deps = pyproject_data['project']['dependencies']

here = os.path.abspath(os.path.dirname(__file__))

version = {}
with open(os.path.join(here, "qi", "_version.py")) as f:
    exec(f.read(), version)

# Get the long description from the README file
with open(os.path.join(here, 'README.rst'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='qi',
    version=version['__version__'],
    description='LibQi Python bindings',
    long_description=long_description,
    long_description_content_type='text/x-rst',
    keywords=['libqi', 'qi', 'naoqi',
              'softbank', 'robotics', 'aldebaran',
              'robot', 'nao', 'pepper', 'romeo'],
    url='https://github.com/aldebaran/libqi-python',
    author='SoftBank Robotics Europe',
    author_email='release@softbankrobotics.com',
    license='BSD 3-Clause License',
    python_requires='~=3.5',
    install_requires=pyproject_deps,
    packages=find_packages(exclude=[
        '*.test', '*.test.*', 'test.*', 'test'
    ]),
    include_package_data=True,
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'License :: OSI Approved :: BSD License',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'Topic :: Software Development :: Libraries :: Application Frameworks',
        'Topic :: Software Development :: Embedded Systems',
        'Framework :: Robot Framework :: Library',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3 :: Only',
    ],
    cmake_args=['-DQIPYTHON_STANDALONE:BOOL=ON'],
)
