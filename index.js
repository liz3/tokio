const { init } = require("./bindings/build/index");
let voiceConn = null;
init(process.env.TOKEN).then((instance) => {
  const { bot } = instance;
  bot.addEventListener("READY", async (result) => {
    console.log(result);
  });
  bot.addEventListener("MESSAGE_CREATE", async (result) => {
    if (result.authorId === "195906408561115137") {
      if (!result.content.startsWith("!play")) return;
      if (voiceConn) {
        voiceConn.playFile(result.content.substr(6));
        return;
      }
      bot
        .getChannel("410458159568650253")
        .then(async (res) => {
          const r = await res.connect();
          r.addEventListener("SESSION_DEP", () => {
            voiceConn = r;
            r.playFile(result.content.substr(6));
          });
          r.connect();
        })
        .catch((err) => {
          console.log(err);
        });
    }
  });

  bot.connect();
});
