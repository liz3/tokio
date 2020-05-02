# Dis Light
This project is still WIP.
C++ Discord library with node frontend

# building
**Note**: Atm theres no windows support. tho adding in the future is not excluded
## prerequisites
* node
* npm
* cmake

## Native deps
Install native dependencies
### macOS
`brew install mad libsodium opus nlohmann-json`
### Gnu/Linux
(This is applied to apt using distros, but should work on all, tested in ubuntu 19.10 docker container)
`apt install nlohmann-json3-dev libmad0-dev libsodium-dev libopus-dev`

## building ixwebsocket

1. `git clone https://github.com/machinezone/IXWebSocket`
2. `cd IXWebSocket && mkdir build`
3. (ONLY Gnu/Linux)  add `set(CMAKE_POSITION_INDEPENDENT_CODE ON)` into IXWebsocket's CMakeLists after `set(CMAKE_MODULE_PATH..`
4. `cd build`
5. `cmake -DUSE_TLS=1 ..`
6. `make -j`
7. `(sudo) make install`

## Building
1. `npm install`
2. `npm run compile`

# Running
After building Simple start the `index.js` with node.


# Libs
- nlohmann/json
- IXWebsocket
- node-addon-api
- opus
- mad
- sodium
