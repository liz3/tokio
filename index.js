const { init } = require("./bindings/build/index");
const fs = require("fs");
const https = require("https");
const http = require("http");
const path = require("path");
let voiceConn = null;
let currentChannel = null;
const voiceStates = {};
init(process.env.TOKEN).then((instance) => {
  const { bot } = instance;
  bot.addEventListener("READY", async (result) => {
    console.log("READY emited", result);
  });
  bot.addEventListener("VOICE_STATE_UPDATE", async (res) => {
    voiceStates[res.raw.user_id] = res.raw;
  });
  bot.addEventListener("GUILD_CREATE", async (res) => {
    res.raw.voice_states.forEach((e) => (voiceStates[e.user_id] = e));
  });
  bot.addEventListener("MESSAGE_CREATE", async (result) => {
    if (
      !result.content.startsWith("!play") &&
      !result.content.startsWith("!disconnect") &&
      !result.content.startsWith("!stop") &&
      !result.content.startsWith("!volume") &&
      !result.content.startsWith("!download")
    )
      return;
    if (result.content.startsWith("!download")) {
      const [cmd, url, file] = result.content.split(";");
      const module = url.startsWith("https") ? https : http;
      const fStream = fs.createWriteStream(path.join("/media-data", file));
      result.channel.send("perfoming download...");
      module.get(url, (response) => {
        response.on("end", () => {
          result.channel.send("successfully saved file!");
        });
        response.pipe(fStream);
      });
      return;
    }
    if (result.content === "!stop") {
      if (voiceConn) {
        voiceConn.stop();
        result.channel.send("Stopping to play");
      }
      return;
    }
    if (result.content.startsWith("!volume")) {
      if (voiceConn) {
        const value = Number.parseFloat(result.content.split(" ")[1], 10);
        if (isNaN(value)) return;
        voiceConn.setGain(value);
        result.channel.send(`Updating volume to ${value}`);
      }
      return;
    }

    if (result.content === "!disconnect") {
      if (voiceConn) {
        voiceConn.disconnect();
        result.channel.send("Disconnecting");
        voiceConn = null;
      }
      return;
    }
    if (voiceConn) {
      voiceConn.playFile(result.content.substr(6));
      return;
    }
    bot
      .getGuild(result.raw.guild_id)
      .then(async (res) => {
        const entry = voiceStates[result.raw.author.id];
        if (!entry) return;
        const channel = res.channels.find((it) => it.id === entry.channel_id);
        if (!channel) return;
        if (currentChannel !== channel.id && voiceConn) {
          voiceConn.disconnect();
          voiceConn = null;
        }
        currentChannel = channel.id;
        const r = await channel.connect();
        r.addEventListener("SESSION_DEP", () => {
          voiceConn = r;
          r.playFile(result.content.substr(6));
        });
        r.connect();
      })
      .catch((err) => {
        console.log(err);
      });
  });

  bot.connect();
});
