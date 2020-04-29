#/bin/sh

git clone https://github.com/machinezone/IXWebSocket
mkdir build
cd build
cmake -DUSE_TLS=1 ..
make -j
make install
