FROM ubuntu:20.04
WORKDIR /bot
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y build-essential git cmake nlohmann-json3-dev libmad0-dev libsodium-dev libopus-dev nodejs npm libopusfile-dev zip unzip python pandoc ffmpeg wget curl
WORKDIR /bot/tokio
COPY . .
RUN git submodule update --init
WORKDIR /bot/tokio/IXWebSocket
RUN mkdir build
WORKDIR /bot/tokio/IXWebSocket/build
RUN cmake -DUSE_TLS=1 ..
RUN make -j
RUN make install
WORKDIR /bot/tokio
RUN npm install
RUN ./youtube-dl.sh
RUN npm run compile
RUN mkdir /media-data
CMD ["node", "index.js"]
