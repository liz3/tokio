const { init } = require("./bindings/build/index");

init(process.env.TOKEN).then((instance) => {
  const { bot } = instance;
  bot.addEventListener("READY", async (result) => {
    console.log(result);
  });
  bot.addEventListener("MESSAGE_CREATE", async (result) => {
    if (result.authorId === "195906408561115137") {
      bot
        .getChannel("410458159568650253")
        .then(async (res) => {
          const r = await res.connect();
          r.addEventListener("SESSION_DEP", () => {
            r.playFile(process.argv[2]);
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
