#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Build a Python SDK Version """
from __future__ import absolute_import
from __future__ import unicode_literals
from __future__ import print_function

import os
import json
import stat
import shutil
import platform

from setuptools import sandbox

PATH_HERE = os.path.dirname(os.path.realpath(__file__))
CLEAN_NAMES = [
    "bin", "lib", "temp", "dist", "build",
    "share", "include", "qi.egg-info",
    "win", "mac", "linux", "robot",
    "setup.cfg", "version.py"
]
CLEAN_EXT = [".pyc", ".dll", ".so", ".pyd", ".dylib"]
SETUP_CFG = """[metadata]
license_file = LICENSE.txt

[bdist_wheel]
python-tag = cp27
plat-name = %s
plat-tag = %s
"""

def clean_access_rights(path):
    """ Allow write access to a path """
    if os.path.isfile(path):
        if not os.access(path, os.W_OK):
            os.chmod(path, stat.S_IWRITE)
            os.chmod(path, stat.S_IWUSR)
    elif os.path.isdir(path):
        if not os.access(path, os.W_OK):
            os.chmod(path, stat.S_IWRITE)
            os.chmod(path, stat.S_IWUSR)
        for dname, dirs, files in os.walk(path):
            for dirname in dirs:
                dirpath = os.path.join(dname, dirname)
                if os.path.isdir(dirpath):
                    if not os.access(dirpath, os.W_OK):
                        os.chmod(dirpath, stat.S_IWRITE)
                        os.chmod(dirpath, stat.S_IWUSR)
            for fname in files:
                fpath = os.path.join(dname, fname)
                if os.path.isfile(fpath):
                    if not os.access(fpath, os.W_OK):
                        os.chmod(fpath, stat.S_IWRITE)
                        os.chmod(fpath, stat.S_IWUSR)

def folder_cleanup(folder, names=None, extensions=None):
    """ Remove all sub folders and files by name or extension """
    if not isinstance(names, list):
        names = []
    if not isinstance(extensions, list):
        extensions = []
    clean_access_rights(folder)
    for dname, dirs, files in os.walk(folder):
        for dirname in dirs:
            if dirname not in names:
                continue
            dirpath = os.path.join(dname, dirname)
            if os.path.isdir(dirpath):
                try:
                    shutil.rmtree(dirpath, ignore_errors=False)
                    print("Removed Folder %s" % dirpath)
                except:
                    print("Failed to remove Folder %s" % dirpath)
        for fname in files:
            fpath = os.path.join(dname, fname)
            if fname in names:
                if os.path.isfile(fpath):
                    try:
                        os.remove(fpath)
                        print("Removed File %s" % fpath)
                    except:
                        print("Failed to remove File %s" % fpath)
            fext = os.path.splitext(fname)[-1]
            if fext in extensions or fname in extensions:
                if os.path.isfile(fpath):
                    try:
                        os.remove(fpath)
                        print("Removed File by Ext %s" % fpath)
                    except:
                        print("Failed to remove File by Ext %s" % fpath)

def create_from_archive():
    """ Create a Package from a downloaded archive """
    build_version = os.environ.get("QIPYTHON_BUILD_VERSION", "2.9.0.999")
    build_platform = os.environ.get("QIPYTHON_BUILD_PLATFORM", platform.system())
    build_folder = os.environ.get("QIPYTHON_BUILD_FOLDER")
    if not build_folder:
        build_folder = PATH_HERE
    # Cleanup the Folder
    folder_cleanup(PATH_HERE, names=CLEAN_NAMES, extensions=CLEAN_EXT)
    # Set the Platform in Environment Variable
    if "mac" in build_platform.lower():
        build_platform = "macosx-10.12-intel"
    elif "win" in build_platform.lower():
        build_platform = "win-amd64"
    elif "linux" in build_platform.lower():
        build_platform = "manylinux1-x86_64"
    else:
        print("SDK Creation cancelled, platform %s is not supported" % build_platform)
        return
    os.environ["QIPYTHON_BUILD_PLATFORM"] = build_platform
    print("Create LibQi Python %s %s" % (build_platform, build_version))
    # Create the setup.cfg File
    with open(os.path.join(PATH_HERE, "setup.cfg"), "w") as setup_file:
        setup_file.write(SETUP_CFG % (build_platform, build_platform))
    setup_file.close()
    # Clean the qi Folder just in in case
    folder_cleanup(os.path.join(PATH_HERE, "qi"), names=["version.py"], extensions=CLEAN_EXT)
    # Create the version.py File
    with open(os.path.join(PATH_HERE, "qi", "version.py"), "w") as version_file:
        version_file.write("#!/usr/bin/env python\n# -*- coding: utf-8 -*-\nversion = \"%s\"" % build_version)
    version_file.close()
    # Load the List of files to Include
    with open(os.path.join(PATH_HERE, "ListFiles.json"), "r") as file_json:
        list_files = json.load(file_json)
    file_json.close()
    # Copy each File if present
    platform_files = list_files.get(build_platform)
    for folder, _, files in os.walk(build_folder):
        for filename in files:
            file_path = os.path.join(folder, filename)
            if filename not in platform_files:
                continue
            path_dest = os.path.join(PATH_HERE, "qi", platform_files.get(filename))
            folder_dest = os.path.dirname(path_dest)
            if not os.path.isdir(folder_dest):
                os.makedirs(folder_dest)
            try:
                shutil.copyfile(file_path, path_dest)
                print("File Copied to %s" % path_dest)
            except:
                print("Unable to copy file %s" % file_path)
    # Build the Package
    sandbox.run_setup(os.path.join(PATH_HERE, "setup.py"), ["bdist_wheel"])
    print("LibQi Python %s %s Created" % (build_platform, build_version))

if __name__ == "__main__":
    create_from_archive()
