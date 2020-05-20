# Tokio

This project is still WIP.
C++ Discord library with node bindings focused on audio playback.

The Core parts as networking with discord/audio decoding and encoding is written in [c++11](https://gitlab.com/HCInk/tokio/-/tree/master/lib).
Theres a thin api wrapper around the native bindings written in [Typescript](https://gitlab.com/HCInk/tokio/-/tree/master/bindings)

## Supported file formats

- MPEG3(mp3)
- opus(.opus in ogg container)
- WAV(experimental!)

## Easy usage.

To use the demo bot simply use the [docker image](https://hub.docker.com/r/liz3/relight)

```sh
docker pull liz3/relight
```

# building

## prerequisites

- [node/npm](https://nodejs.org/en/)
- [cmake](https://cmake.org/download/)

## Windows

First of install [windows-build-toold](https://www.npmjs.com/package/windows-build-tools)
(Admin cmd/Powershell)

```
npm install --global --production windows-build-tools
```

Then also install [vcpkg](https://github.com/microsoft/vcpkg#quick-start)

## Native deps

Install native dependencies

### macOS

`brew install mad libsodium opus opusfile nlohmann-json`

Then simply: `./setup.sh`

### Gnu/Linux

(This is applied to apt using distros, but should work on all, tested in ubuntu 19.10 docker container)
`apt install nlohmann-json3-dev libmad0-dev libsodium-dev libopus-dev libopusfile-dev`

Then simply: `./setup.sh`

### Windows

Run this in the folder where you installed vcpkg

```
> vcpkg install libsodium:x64-windows
> vcpkg install opusfile:x64-windows
> vcpkg install opus:x64-windows
> vcpkg install ixwebsocket[mbedtls]:x64-windows
> vcpkg install nlohmann-json:x64-windows
> vcpkg install libmad:x64-windows
> vcpkg install openssl:x64-windows
```

## Setup

### macOS & GNU/Linux

1. `npm install`
2. `npm run compile`

### Windows

(Make sure to use forward slashes!, use git bash for the sake of inline env vars)

1. `npm install`
2. `VCPKG_ROOT=/path/to/vcpkg-root npm run compile`, Example: `VCPKG_ROOT=C:/Users/liz3/Desktop/vcpkg npm run compile`

# Running

After building, in order to run you can use the `npm start` script.

## macOS & GNU/Linux

```sh
TOKEN=my_discord_token npm start
```

## Windows

(Use git bash)

```sh
TOKEN=my_discord_token npm start
```

## Youtube dl

To add Youtube dl which is needed by the web fetcher, simply run the script to fetch it. The script will clone and build youtube-dl.

(Make sure you have [python](https://www.python.org/downloads/))

`./youtube-dl.sh`

# Libs

- nlohmann/json
- IXWebsocket
- node-addon-api
- opus
- mad
- sodium
- opusfile

# LICENSE

Tokio is free software licensed under [GPL 2.0](https://gitlab.com/HCInk/tokio/-/tree/master/LICENSE)
