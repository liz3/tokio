const dis_light = require("bindings")("dis_light");
import { generateTextChannel, generateVoiceChannel } from "../impl/channel";
import { getGuild } from "../impl/guild";
interface Instance {
  readonly version: string;
  readonly bot: Bot;
}

export default function init(token: string): Promise<Instance> {
  return new Promise((resolve, reject) => {
    dis_light.init(token, (err, bot) => {
      if (err) {
        console.log(err);
        reject(err);
        return;
      }
      const instance: Instance = {
        version: "1.0",
        bot: {
          connect: () => bot.connect(),
          close: () => {
            bot.close();
          },
          getGuild: (guildId) => {
            return getGuild(guildId, bot);
          },
          getChannel: (id) => {
            return new Promise((resolve, reject) => {
              bot.getChannelStandalone(id, (success, data) => {
                if (!success) {
                  reject(JSON.parse(data));
                  return;
                }
                const parsed = JSON.parse(data);
                if (parsed.type === 0) {
                  resolve(generateTextChannel(id, "", bot));
                } else if (parsed.type === 2) {
                  resolve(generateVoiceChannel(id, parsed.guild_id, bot));
                } else {
                  const channel: Channel = {
                    id,
                  };
                  resolve(channel);
                }
              });
            });
          },
          addEventListener: (name, handler) => {
            bot.addEventListener(name.toString(), (data) => {
              if (name == "MESSAGE_CREATE") {
                const message: MessageCreateEvent = {
                  channel: generateTextChannel(
                    data.channel_id,
                    data.guild_id,
                    bot
                  ),
                  messageId: data.id,
                  raw: data,
                  content: data.content,
                  authorId: data.author.id,
                };
                handler(message);
                return;
              }
              const response: EventResponse = {
                raw: data,
              };
              handler(response);
            });
          },
        },
      };
      resolve(instance);
    });
  });
}
