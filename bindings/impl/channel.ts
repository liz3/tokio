import { prepareFile } from "./youtube-dl";
import { existsSync, unlink } from "fs";
const cache = {};
const voiceCache = {};

let currentFile;

const clearCurrentFile = () => {
  if (currentFile) {
    if (existsSync(currentFile)) {
      unlink(currentFile, () => {});
    }
  }
};

export function generateTextChannel(
  channelId: string,
  guildId: string,
  bot: any
) {
  if (cache[channelId]) return cache[channelId];
  const channel: TextChannel = {
    id: channelId,
    guildId,
    send: (payload) => {
      let fPayload;
      if (typeof payload !== "string") fPayload = payload;
      else fPayload = { content: payload };
      return new Promise((resolve, reject) => {
        bot.sendChannelMessage(
          channelId,
          JSON.stringify(fPayload),
          (sucess, data) => {
            if (!sucess) {
              const result: ResponsePayload = {
                success: false,
                data,
              };
              reject(result);
              return;
            }
            const result: ResponsePayload = {
              success: true,
              data: JSON.parse(data),
            };
            resolve(result);
          }
        );
      });
    },
  };
  cache[channelId] = channel;
  return channel;
}

export function generateVoiceChannel(channelId: string, guildId: string, bot) {
  if (voiceCache[channelId]) return voiceCache[channelId];

  const channel: VoiceChannel = {
    id: channelId,
    connect: () => {
      return new Promise((resolve) => {
        bot.connectVoice(guildId, channelId, (result) => {
          const connection: VoiceConnection = {
            connect: () => {
              result.connect();
              connection.connected = true;
            },
            connected: false,
            playing: false,
            addEventListener: (name, handler) => {
              result.addEventListener(name.toString(), (data) => {
                const w: EventResponse = {
                  raw: data,
                };
                handler(w);
              });
            },
            disconnect: () => {
              clearCurrentFile();
              result.disconnect();
              connection.connected = false;
            },
            setGain(value: number) {
              result.setGain(value);
            },
            stop: () => {
              clearCurrentFile();
              result.stop();
              connection.playing = false;
            },
            playFile: (path: string) => {
              clearCurrentFile();
              const lower = path.toLowerCase();
              if (lower.endsWith(".opus")) {
                connection.playing = true;
                result.playOpusFile(path);
              } else if (lower.endsWith(".mp3")) {
                connection.playing = true;

                result.playFile(path);
              } else if (lower.endsWith(".wav")) {
                connection.playing = true;

                result.playWavFile(path);
              } else if (
                lower.startsWith("http://") ||
                lower.startsWith("https://")
              ) {
                prepareFile(path)
                  .then((resultData) => {
                    const match = resultData.formats
                      .filter(
                        (entry) =>
                          entry.acodec === "opus" && entry.vcodec === "none"
                      )
                      .sort((a, b) => b.filesize - a.filesize);
                    if (match.length) result.playPiped(0, match[0].url);
                  })
                  .catch((err) => console.log(err));
              }
            },
          };
          resolve(connection);
        });
      });
    },
  };
  voiceCache[channelId] = channel;
  return channel;
}
