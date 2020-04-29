const dis_light = require("bindings")("dis_light");
dis_light.init(process.env.TOKEN, (err, bot) => {

  bot.addEventListener("READY", data => {
    console.log("ready received");
  });
  bot.addEventListener("MESSAGE_CREATE", function (data){
    console.log("Message event", data);
  });
  bot.connect();
});
