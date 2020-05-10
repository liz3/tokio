enum GatewayEvent {
  READY = "READY",
  //MESSAGES
  MESSAGE_CREATE = "MESSAGE_CREATE",
  MESSAGE_UPDATE = "MESSAGE_UPDATE",
  MESSAGE_DELETE = "MESSAGE_DELETE",
}
enum VoiceEvent {
  GENERIC = "GENERIC",
  READY = "READY",
  //MESSAGES
  SESSION_DEP = "SESSION_DEP",
  FINISHED_PLAYING = "FINISH_PLAY",
}

interface EventResponse {
  raw: any;
}
interface ResponsePayload {
  success: boolean;
  data: any;
}
//channels
interface Channel {
  id: string;
}
interface VoiceConnection {
  addEventListener(name: VoiceEvent, callback: (EventResponse) => void);
  connect(): void;
  playFile(path: string);
  playing: boolean;
  connected: boolean;
  stop(): void;
  disconnect(): void;
}

interface Bot {
  close(): void;
  addEventListener(name: GatewayEvent, callback: (EventResponse) => void);
  connect(): void;
}
interface TextChannel extends Channel {
  send(payload: any): Promise<ResponsePayload>;
}
interface VoiceChannel extends Channel {
  connect(): Promise<VoiceConnection>;
  connected: boolean;
}

interface GuildChannelsPayload extends ResponsePayload {
  success: boolean;
  data: Channel[];
}

//messages
interface MessageCreateEvent extends EventResponse {
  messageId: string;
  channel: TextChannel;
}
