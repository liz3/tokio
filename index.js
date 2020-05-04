const dis_light = require("bindings")("dis_light");
dis_light.init(process.env.TOKEN, (err, bot) => {
  bot.addEventListener("READY", data => {
    console.log("ready received");
    setTimeout(() => {
      bot.connectVoice("98351956925349888", "410458159568650253", (result) => {
        result.addEventListener("FINISH_PLAY", () => {
          result.disconnect();
        })
        result.addEventListener("SESSION_DEP", () => {
          const path = process.argv[2]
          if(path.toLowerCase().endsWith("mp3")){
            result.playFile(process.argv[2]);
          }else if(path.toLowerCase().endsWith("opus")) {
            result.playOpusFile(process.argv[2])
          } else {
            console.log("unsupported file format");
            return;
          }
        })
        result.addEventListener("READY", () => {
          console.log("ready fired");
        })
        result.connect();
      });
    }, 450);
  });
  bot.addEventListener("MESSAGE_CREATE", function (data){
    console.log("Message event", data);
  });
  bot.connect();
});
