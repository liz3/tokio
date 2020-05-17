#include "dis_websocket.h"
using json = nlohmann::json;
DisWebsocket::DisWebsocket(std::string& socket_url,std::string &token, bool auto_connect) {
  this->webSocket.setUrl(socket_url);
  this->webSocket.disablePerMessageDeflate();
  #ifdef _WIN32 
  ix::SocketTLSOptions opt;
  opt.caFile = std::string(std::getenv("TOKIO_CERTPATH"));
  this->webSocket.setTLSOptions(opt);
  #endif
  this->seq = 0;
  this->token = token;
  zlib_ctx = std::make_unique<zstr::istream>(message_stream);
  auto finalThis = this;
  webSocket.setOnMessageCallback([finalThis](const ix::WebSocketMessagePtr& msg)
                                 {
                                   if(msg->type == ix::WebSocketMessageType::Error) {
                                     std::cout <<  "Connection error: " << msg->errorInfo.reason << std::endl;
                                     return;
                                   }

                                    if (msg->type == ix::WebSocketMessageType::Open) {
                                      if(finalThis->resumed) {
                                        json f;
                                        f["token"] = finalThis->token;
                                        f["session_id"] = finalThis->session_id;
                                        f["seq"] = finalThis->seq;
                                        finalThis->sendMessage(6, f);
                                        finalThis->resumed = false;
                                      }
                                     return;
                                   }

                                   if(msg->type == ix::WebSocketMessageType::Close) {
                                     if(finalThis->running) {
                                       finalThis->webSocket.close();
                                       finalThis->running = false;
                                       finalThis->resume();
                                     }
                                   }
                                   if (msg->type != ix::WebSocketMessageType::Message)
                                     return;

                                   if(!msg->binary) {
                                     finalThis->messageHandler(std::move(msg->str));
                                     return;
                                   }
                                   std::string payload;
                                   auto message_stream = &finalThis->message_stream;
                                   *message_stream << msg->str;
                                   if(std::strcmp((msg->str.data() + msg->str.size() - 4), "\x00\x00\xff\xff"))
                                     return;
                                   std::stringstream ss;
                                   std::string s;
                                   finalThis->zlib_ctx->clear();
                                   while (getline(*finalThis->zlib_ctx, s))
                                     ss << s;
                                   ss << '\0';
                                   payload = ss.str();
                                   finalThis->messageHandler(std::move(payload));
                                 }
    );

  if(auto_connect) {
    this->webSocket.setPingInterval(45);
    this->webSocket.start();
    this->running = true;
  }
}
void DisWebsocket::resume() {
  this->resumed = true;
  this->webSocket.start();
  this->running = true;
}
void DisWebsocket::close() {
  if(!this->running) return;
  this->running = false;
  this->webSocket.close();
}
void DisWebsocket::externalClose(const Napi::CallbackInfo& info) {
  this->close();
}
void DisWebsocket::sendMessage(int opcode, json data) {
  if(!this->running) return;
  json f;
  f["op"] = opcode;
  f["d"] = data;
  this->webSocket.send(f.dump());
}
void DisWebsocket::connect() {
  if(this->running) return;
  this->webSocket.setPingInterval(45);
  this->webSocket.start();
  this->running = true;
}
void DisWebsocket::sendHeartBeat() {
  json f;
  f["op"] = 1;
  f["d"] = this->seq;
  this->webSocket.send(f.dump());
}
void  DisWebsocket::setupHeartBeatInterval(DisWebsocket* instance, int interval) {
  while(instance->running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    if(!instance->running) break;
    instance->sendHeartBeat();
  }
}
void DisWebsocket::registerEventListener(std::string& name, Napi::Function callback) {
  auto ref = Napi::Persistent(callback);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Resource Name", 0, 1);
  DisEventListener listener(tsfn, name);
  event_handlers.push_back(listener);
}
void DisWebsocket::handleVoiceInit(std::string& server_id, std::string& channel_id, const VoiceInitCallback& cb) {
  if(waitingVoiceConnect) return;
  voiceInitCallback = cb;
  waitingVoiceConnect = true;

  json init_obj;
  init_obj["guild_id"] = server_id;
  init_obj["channel_id"] = channel_id;
  init_obj["self_mute"] = false;
  init_obj["self_deaf"] = false;
  init_obj["self_video"] = false;
  sendMessage(4, init_obj);

}
void DisWebsocket::handleVoiceLeave() {
  json init_obj;
  init_obj["guild_id"] = "";
  init_obj["channel_id"] = "";
  init_obj["self_mute"] = false;
  init_obj["self_deaf"] = false;
  init_obj["self_video"] = false;
  sendMessage(4, init_obj);

}

void DisWebsocket::handleAuth() {
  json props;
  props["os"] = "Linux";
  props["browser"] = "";
  props["device"] = "dis_light_driver";
  json data;
  data["properties"] = props;
  data["v"] = 6;
  data["compress"] = false;
  data["token"] = this->token;
  this->sendMessage(2, data);
}
void DisWebsocket::messageHandler(const std::string& msg) {
    this->seq++;
    auto parsed = json::parse(msg);
    int messageOpCode = parsed["op"];
    switch(messageOpCode) {
    case 0: {
      if (parsed["t"].is_string()) {
        std::string evstr = std::string(parsed["t"]);
        if(waitingVoiceConnect) {
          if(evstr.compare("VOICE_SERVER_UPDATE") == 0) {
            if(voice_states.size() == 1) {
              voiceInitCallback(parsed["d"], voice_states[0]);
              waitingVoiceConnect = false;
              voice_states.clear();
            } else {
              voice_states.push_back(parsed["d"]);
            }
            return;
          } else if(evstr.compare("VOICE_STATE_UPDATE") == 0) {
            if(voice_states.size() == 1) {
              voiceInitCallback(voice_states[0], parsed["d"]);
              waitingVoiceConnect = false;
              voice_states.clear();
            } else {
              voice_states.push_back(parsed["d"]);
            }
            return;
          }
        }
         if(evstr.compare("READY") == 0) {
           this->own_id = parsed["d"]["user"]["id"];
           this->session_id = parsed["d"]["session_id"];
         }
        for(auto listener : event_handlers) {
          if(evstr.compare(listener.evName) == 0) {
              auto callback = [msg]( Napi::Env env, Napi::Function jsCallback) {

                                Napi::Object global = env.Global().As<Napi::Object>();
                                Napi::Function parseFunc = global.Get("JSON").As<Napi::Object>().Get("parse").As<Napi::Function>();
                                Napi::Value val = parseFunc.Call(env.Global(), {Napi::String::New(parseFunc.Env(), msg.c_str())});
                                Napi::Object asObject = val.As<Napi::Object>();
                                auto obj = asObject.Get("d").As<Napi::Object>();
                                jsCallback.Call({ obj } );
            };
            auto status = listener.function.BlockingCall( callback );
            if(status != napi_ok) {
              std::cout << "not ok\n";
            }
          }
        }
      }
      break;
    }
    case 1: {
      this->sendHeartBeat();
      break;
    }
    case 10: {
      this->running = true;
      int interval = parsed["d"]["heartbeat_interval"];
      std::thread second (DisWebsocket::setupHeartBeatInterval,this, interval);
      second.detach();
      this->handleAuth();
      break;
    }
    }
}
