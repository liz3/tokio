#!/bin/bash
mkdir yt-dl
cd yt-dl
git clone https://github.com/ytdl-org/youtube-dl.git
cd youtube-dl
make
cp youtube-dl ../../
cd ../../
rm -rf yt-dl
