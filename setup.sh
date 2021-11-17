#!/bin/sh
git submodule update --init
cd IXWebSocket
mkdir build && cd build
cmake -DUSE_TLS=1 ..
make -j
if [ $(uname) == "Darwin" ]
then
    sudo make install
else
    sudo make install
fi
cd ../../
npm install
