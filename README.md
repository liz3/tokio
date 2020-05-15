# Tokio

This project is still WIP.
C++ Discord library with node frontend

## Easy usage.

To use the demo bot simply use the [docker image](https://hub.docker.com/r/liz3/relight)

```sh
docker pull liz3/relight
```

# building

**Note**: Atm theres no windows support. tho adding in the future is not excluded

## prerequisites

- node
- npm
- cmake

## Native deps

Install native dependencies

### macOS

`brew install mad libsodium opus opusfile nlohmann-json`

### Gnu/Linux

(This is applied to apt using distros, but should work on all, tested in ubuntu 19.10 docker container)
`apt install nlohmann-json3-dev libmad0-dev libsodium-dev libopus-dev libopusfile-dev`

## Setup

Simply Run: `./setup.sh`

## Building

1. `npm install`
2. `npm run compile`

# Running

After building Simple start the `index.js` with node.

## Youtube dl

To add Youtube dl which is needed by the web fetcher, simply run the script to fetch it. The script will clone and build youtube-dl.

`./youtube-dl.sh`

# Libs

- nlohmann/json
- IXWebsocket
- node-addon-api
- opus
- mad
- sodium
- opusfile
