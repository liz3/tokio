import { generateTextChannel, generateVoiceChannel } from "./channel";

const guildCache = {};
export function getGuild(guildId, bot): Promise<Guild> {
  if (guildCache[guildId] && guildCache[guildId].time > Date.now() + 120)
    return Promise.resolve(guildCache[guildId].value);
  return new Promise((resolve, reject) => {
    bot.getGuildStandalone(guildId, (success, data) => {
      if (!success) {
        reject(data);
        return;
      }
      bot.getGuildChannels(guildId, (succ2, channels) => {
        if (!succ2) {
          reject(channels);
          return;
        }
        const parsed = JSON.parse(data);
        console.log(parsed.voice_states);
        const guild: Guild = {
          id: guildId,
          voice_states: parsed.voice_states,
          channels: JSON.parse(channels).map((channel) => {
            if (channel.type === 0) {
              return generateTextChannel(channel.id, guildId, bot);
            } else if (channel.type === 2) {
              return generateVoiceChannel(channel.id, guildId, bot);
            }
            const ch: Channel = {
              id: channel.id,
            };
            return ch;
          }),
        };
        resolve(guild);
      });
    });
  });
}
