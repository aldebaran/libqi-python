#!/usr/bin/env python2
##
## Author(s):
##  - Cedric GESTES <gestes@aldebaran.com>
##
## Copyright (C) 2014 Aldebaran
##

""" Inspect qi modules
"""

import time
import sys
import qi
import argparse

def main():
    """ Entry point of qiservice
    """
    app = qi.Application(sys.argv, raw=True)


    parser = argparse.ArgumentParser(description='Inspect qi module')
    parser.add_argument('--module', '-m', help='module name')
    parser.add_argument("--list", "-l", action="store_true", default=False, help="list all available modules")
    args = parser.parse_args()

    if args.list:
        print "modules:"
        for m in qi.listModules():
            print m
        return 0

    if args.list:
        print "modules:"
        for m in qi.listModules():
            print m
        return 0


    print "module:", args.module
    print "prefixes:", qi.path.sdkPrefixes()

    mod = qi.module(args.module)
    for o in mod.objects():
        print "object:", o
    for f in mod.functions():
        print "func  :", f
    for c in mod.constants():
        print "const :", c

if __name__ == "__main__":
    main()
