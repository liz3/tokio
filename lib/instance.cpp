#include "instance.h"
Instance::Instance(std::string token) {
  this->token = token;
}
Napi::Object Instance::generateBindings(Napi::Env env) {
  Napi::Object obj = Napi::Object::New(env);
  auto socket = this->socket;
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
   obj.Set("connect", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {

                                                         socket->connect();
                                                       }
      ));
  return obj;
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
