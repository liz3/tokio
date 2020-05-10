const disLight = requre("dis-light");

// const dis_light = require("bindings")("dis_light");
// dis_light.init(process.env.TOKEN, (err, bot) => {
//   bot.addEventListener("READY", data => {
//     console.log("ready received");
//     setTimeout(() => {
//       bot.connectVoice("98351956925349888", "410458159568650253", (result) => {
//         result.addEventListener("FINISH_PLAY", () => {
//           result.disconnect();
//         })
//         result.addEventListener("SESSION_DEP", () => {
//           console.log("session dep")
//           const path = process.argv[2]
//           if(path.toLowerCase().endsWith("mp3")){
//             result.playFile(process.argv[2]);
//           }else if(path.toLowerCase().endsWith("opus")) {
//             result.playOpusFile(process.argv[2])
//           } else {
//             console.log("unsupported file format");
//             return;
//           }
//         })
//         result.addEventListener("READY", () => {
//           console.log("ready fired loolz");
//         })
//         result.connect();
//       });
//     }, 450);
//     setTimeout(() => {
//       bot.connectVoice("609022153467101186", "609022154213949463", (result) => {
//         result.addEventListener("FINISH_PLAY", () => {
//           result.disconnect();
//         })
//         result.addEventListener("SESSION_DEP", () => {
//           const path = process.argv[2]
//           if(path.toLowerCase().endsWith("mp3")){
//             result.playFile(process.argv[2]);
//           }else if(path.toLowerCase().endsWith("opus")) {
//             result.playOpusFile(process.argv[2])
//           } else {
//             console.log("unsupported file format");
//             return;
//           }
//         })
//         result.addEventListener("READY", () => {
//           console.log("ready fired");
//         })
//         console.log("attempting second connect");
//         result.connect();
//       });
//     }, 1000);

//     setTimeout(() => {
//       bot.editChannelMessage("609022153467101186","609022154213949463", JSON.stringify({ content: "Edited Message"}), (success, data) => {
//       console.log(success, data);
//     })
//     }, 500)
//   });
//   bot.addEventListener("MESSAGE_CREATE", function (data){
//   //  console.log("Message event", data);
//   });
//   bot.connect();
// });
