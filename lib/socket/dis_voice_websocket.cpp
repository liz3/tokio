
#include "dis_voice_websocket.h"
DisVoiceWebsocket::DisVoiceWebsocket(const json& server_state, const json& voice_state, std::string& id, std::string& server_id, std::string& channel_id) {
  std::cout <<  "  LOOL" <<  server_id <<  "\n";
  this->dis_id = id;
  this->server_id = server_id;
  this->channel_id = channel_id;
  std::string target_endpoint = "wss://"+ std::string(server_state["endpoint"]).substr(0, std::string(server_state["endpoint"]).length() -3) + "/?v=4";
  std::cout << target_endpoint << "\n";
  this->webSocket.setUrl(target_endpoint);
  this->session_id = voice_state["session_id"];
  this->webSocket.disablePerMessageDeflate();
  this->seq = 0;
  this->token = server_state["token"];
  zlib_ctx = std::make_unique<zstr::istream>(message_stream);
  auto finalThis = this;
  std::cout << "reached callback\n";

  webSocket.setOnMessageCallback([finalThis](const ix::WebSocketMessagePtr& msg)
                                 {
                                   if(msg->type == ix::WebSocketMessageType::Error) {
                                     std::cout <<  "Connection error: " << msg->errorInfo.reason << std::endl;
                                     return;
                                   }
                                   if (msg->type == ix::WebSocketMessageType::Open) {
                                     finalThis->handleAuth();
                                     return;
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


  //  this->webSocket.setPingInterval(45);
    this->webSocket.start();
    this->running = true;

}
void DisVoiceWebsocket::close() {
  if(!this->running) return;
  this->webSocket.close();
  this->running = false;
}
void DisVoiceWebsocket::externalClose(const Napi::CallbackInfo& info) {
  this->close();
}
void DisVoiceWebsocket::sendMessage(int opcode, json data) {
  if(!this->running) return;
  json f;
  f["op"] = opcode;
  f["d"] = data;
  this->webSocket.send(f.dump());
}
void DisVoiceWebsocket::connect() {
  if(this->running) return;
  this->webSocket.setPingInterval(45);
  this->webSocket.start();
  this->running = true;
}
void DisVoiceWebsocket::sendHeartBeat() {
  json f;
  f["op"] = 3;
  f["d"] = this->seq;
  this->webSocket.send(f.dump());
}
void  DisVoiceWebsocket::setupHeartBeatInterval(DisVoiceWebsocket* instance, int interval) {
  while(instance->running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    if(!instance->running) break;
    instance->sendHeartBeat();
  }
}
void DisVoiceWebsocket::registerEventListener(std::string& name, Napi::Function callback) {
  auto ref = Napi::Persistent(callback);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Resource Name", 0, 1);
  DisEventListener listener(tsfn, name);
  event_handlers.push_back(listener);
}
void DisVoiceWebsocket::handleAuth() {
  json data;
  data["token"] = this->token;
  data["server_id"] = this->server_id;
  data["user_id"] = this->dis_id;
  data["session_id"] = this->session_id;
  std::cout << "sending authenticate: " << data.dump() << "\n";
  this->sendMessage(0, data);
}
void DisVoiceWebsocket::updateSpeakingState(bool speaking) {
  json status;
  status["speaking"] = speaking ? 1 : 0;
  status["delay"] = 0;
  status["ssrc"] = this->ssrc;
  this->sendMessage(5, status);
}
void DisVoiceWebsocket::messageHandler(const std::string& msg) {

    this->seq++;
    auto parsed = json::parse(msg);
    int messageOpCode = parsed["op"];
    switch(messageOpCode) {
    case 2:
    case 4:
    case 5: {
       if(messageOpCode ==2) {
         std::cout << "received ready!!\n";
         std::cout <<  msg << "\n";

         auto data = parsed["d"];
         this->ssrc = data["ssrc"];
         this->port = data["port"];
         this->ip = data["ip"];
         if(this->voiceConn == nullptr) {
           this->voiceConn = new VoiceConnection(this->ip, this->port, this->ssrc);
           if(this->voiceConn->setupAndHandleSocket()) {
             json f;
             f["protocol"] = "udp";
             json data;
             data["address"] = this->voiceConn->own_ip;
             data["port"] = this->voiceConn->own_port;
             data["mode"] = "xsalsa20_poly1305";
             f["data"] = data;
             std::cout << "sending auth message" <<  f.dump() << "\n";
             this->sendMessage(1, f);
             json meta;
             meta["audio_ssrc"] = this->ssrc;
             meta["video_ssrc"] = 0;
             meta["rtx_ssrc"] = 0;
             this->sendMessage(12, meta);
           }
         }
         return;
       }
       if(messageOpCode == 4) {
         std::cout << "received auth confirm!!\n";
         std::cout <<  msg << "\n";
         // ready with key
         this->voiceConn->startHeartBeat(this->heart_beat_interval);
         json token = parsed["d"]["secret_key"];
         std::vector<unsigned char> key;
         for(int i = 0; i < token.size(); i++)  {
          key.push_back(token[i].get<unsigned char>() & 0xFF);
         }
         this->voiceConn->key = key;

       //  std::cout << std::string(key) << "\n";
       //   this->voiceConn->key = key;
         auto finalThis = this;
           std::thread t([finalThis](){
                           std::cout << "starting offthread\n";
                           finalThis->updateSpeakingState(true);
                           finalThis->voiceConn->play_test();
                         });
           t.detach();
           return;
       }
       if (parsed["t"].is_string()) {
         std::string evstr = std::string(parsed["t"]);
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
    case 8: {
      this->running = true;
      int interval = parsed["d"]["heartbeat_interval"];
      heart_beat_interval = interval;

      std::thread second (DisVoiceWebsocket::setupHeartBeatInterval,this, interval);
      second.detach();

      break;
    }
    }
}
