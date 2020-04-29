
#include "dis_websocket.h"
DisWebsocket::DisWebsocket(std::string& socket_url,std::string &token, bool auto_connect) {
  this->webSocket.setUrl(socket_url);
  this->webSocket.disablePerMessageDeflate();
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
                                   if (msg->type != ix::WebSocketMessageType::Message)
                                     return;

                                   if(!msg->binary) {
                                     std::cout << "received non binary message\n";
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
void DisWebsocket::close() {
  if(!this->running) return;
  this->webSocket.close();
  this->running = false;
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
 //   DisEventListener listener(name, env, callback);
  auto ref = Napi::Persistent(callback);
  auto tsfn = Napi::ThreadSafeFunction::New(
                                            ref.Env(),
                                            ref.Value(),  // JavaScript function called asynchronously
                                            "Resource Name",         // Name
                                            0,                       // Unlimited queue
                                            1                      // Only one thread will use this initiall
                                            );

  DisEventListener listener(tsfn, name);
  event_handlers.push_back(listener);
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
              }            }
          }
        }
        break;
    }
    case 1: { //send heartbeat
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
