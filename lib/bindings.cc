#include <napi.h>
#include <iostream>
#include "instance.h"
#include <ixwebsocket/IXNetSystem.h>
#include <nlohmann/json.hpp>
#include <unistd.h>


static void init(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();
  if (info.Length() != 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
      .ThrowAsJavaScriptException();
    return;
  }
  Napi::Function callback = info[1].As<Napi::Function>();
  std::string passed_token = info[0].As<Napi::String>().Utf8Value();
  Instance* instance = new Instance(passed_token);
  int created = instance->bootstrap();
  if(created != 0) {
    callback.Call(env.Global(), {Napi::Number::New(env, created), env.Null()});
    return;
  }
  Napi::Object instanceArgs = instance->generateBindings(env);
  callback.Call(env.Global(), {env.Null(), instanceArgs});

}
static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  ix::initNetSystem();
  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, init));
  return exports;
}

NODE_API_MODULE(dis_light, Init)
