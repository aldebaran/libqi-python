#!/bin/bash

install=$HOME/opt/boost_1_64_0

wget -q http://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.gz/download -O boost_1_64_0.tar.gz || exit -1
tar xzf boost_1_64_0.tar.gz || exit -1
cd boost_1_64_0
./bootstrap.sh --prefix=$install --with-libraries=atomic,date_time,thread,chrono,filesystem,locale,regex,program_options,random || exit -1
./b2 -d0 link=shared threading=multi variant=release install || exit -1

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}

if [[ "$OSTYPE" == "darwin"* ]]
then
    find $HOME/opt/boost_1_64_0/lib -name \*.dylib | while read dylib
    do
        dylibpath=$(realpath "$dylib")
        install_name_tool -id "$dylibpath" "$dylibpath"
        install_name_tool -change libboost_system.dylib "$install/lib/libboost_system.dylib" "$dylibpath"
    done || exit -1
fi
