#include "instance.h"
using json = nlohmann::json;
Instance::Instance(std::string token) {
  this->token = token;
}
Napi::Object Instance::generateBindings(Napi::Env env) {
  Napi::Object obj = Napi::Object::New(env);
  auto socket = this->socket;
  auto vc_socket = this->voice_sock;
  Instance* finalThis = this;
  obj.Set("version", "1.0");
  obj.Set("close", Napi::Function::New(env, [socket, finalThis](const Napi::CallbackInfo& info) {
                                              if(finalThis->hold) finalThis->hold = false;
                                              socket->externalClose(info);
                                            }
                                        ));
  obj.Set("addEventListener", Napi::Function::New(env, [socket, finalThis, env](const Napi::CallbackInfo& info) {
                                                         if (info.Length() != 2) {
                                                           Napi::TypeError::New(env, "Wrong number of arguments")
                                                             .ThrowAsJavaScriptException();
                                                           return;
                                                         }
                                                         std::string eventName = info[0].As<Napi::String>().Utf8Value();
                                                         Napi::Function callback = info[1].As<Napi::Function>();
                                                         socket->registerEventListener(eventName, callback);
                                                       }
      ));
  obj.Set("connectVoice", Napi::Function::New(env, [finalThis,socket, vc_socket, env](const Napi::CallbackInfo& info) {
                                                      if (info.Length() != 3) {
                                                           Napi::TypeError::New(env, "Wrong number of arguments")
                                                             .ThrowAsJavaScriptException();
                                                           return;
                                                         }
                                                         std::string server_id = info[0].As<Napi::String>().Utf8Value();
                                                         std::string channel_id = info[1].As<Napi::String>().Utf8Value();
                                                         Napi::Env tEnv = info.Env();
                                                         Napi::Function callback = info[2].As<Napi::Function>();
                                                         finalThis->generateVoiceBindings(tEnv, callback, server_id, channel_id,socket, vc_socket);
                                              }
      ));
  obj.Set("connect", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {

                                                socket->connect();
                                              }
      ));
  return obj;
}
void Instance::generateVoiceBindings(Napi::Env env, Napi::Function callback, std::string& server_id, std::string& channel_id, DisWebsocket* sock, DisVoiceWebsocket* vc_socket) {
  if(vc_socket != nullptr) {

    return;
  }
  auto ref = Napi::Persistent(callback);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Audio creation callback", 0, 1);

  sock->handleVoiceInit(server_id, channel_id, [vc_socket, sock, server_id, channel_id, tsfn](const json& server_state, const json& voice_state) mutable {
                                                 vc_socket = new DisVoiceWebsocket(server_state, voice_state, sock->own_id, server_id, channel_id);
                                                 auto callback = []( Napi::Env env, Napi::Function jsCallback, DisVoiceWebsocket* socket) {
                                                                   Napi::Object obj = Napi::Object::New(env);
                                                                     obj.Set("connect", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
                                                                                                                   socket->connect();
                                                                                                                 }
                                                                         ));
                                                                     obj.Set("addEventListener", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
                                                                                                                            auto env = info.Env();
                                                                                                                            if (info.Length() != 2) {
                                                                                                                              Napi::TypeError::New(env, "Wrong number of arguments")
                                                                                                                                .ThrowAsJavaScriptException();
                                                                                                                              return;
                                                                                                                            }
                                                                                                                            std::string eventName = info[0].As<Napi::String>().Utf8Value();
                                                                                                                            Napi::Function callback = info[1].As<Napi::Function>();
                                                                                                                            socket->registerEventListener(eventName, callback);
                                                                                                                          }
                                                                         ));
                                                                     obj.Set("playFile", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
                                                                                                                    auto env = info.Env();
                                                                                                                    if (info.Length() != 1) {
                                                                                                                      Napi::TypeError::New(env, "Wrong number of arguments")
                                                                                                                        .ThrowAsJavaScriptException();
                                                                                                                      return;
                                                                                                                    }
                                                                                                                    std::string path = info[0].As<Napi::String>().Utf8Value();
                                                                                                                    socket->playFile(path);
                                                                                                                  }
                                                                         ));

                                                                     jsCallback.Call({ obj } );
                                                                 };
                                                 auto status = tsfn.BlockingCall( vc_socket, callback );
                                                 if(status != napi_ok) {
                                                   std::cout << "not ok\n";
                                                 }
                                               });
}

int Instance::bootstrap() {
  discord_simple_response response = discord_get_gateway_endpoint(this->token);
  if(!response.success) {
    return 1;
  }
  std::string final_url = std::string(response.value +  "/?compress=zlib-stream&v=6&encoding=json");
  this->socket = new DisWebsocket(final_url, this->token, false);
  return 0;
};
