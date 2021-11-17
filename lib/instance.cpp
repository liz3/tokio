#include "instance.h"
using json = nlohmann::json;
Instance::Instance(std::string& token) {
  this->token = token;
}
Napi::Object Instance::generateBindings(Napi::Env env) {
  auto token = this->token;
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
  obj.Set("connectVoice", Napi::Function::New(env, [finalThis,socket,  env](const Napi::CallbackInfo& info) {
    if (info.Length() != 3) {
      Napi::TypeError::New(env, "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string server_id = info[0].As<Napi::String>().Utf8Value();
    std::string channel_id = info[1].As<Napi::String>().Utf8Value();
    Napi::Env tEnv = info.Env();
    Napi::Function callback = info[2].As<Napi::Function>();
    finalThis->generateVoiceBindings(tEnv, callback, server_id, channel_id,socket);
  }
                                             ));
  obj.Set("sendChannelMessage", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 3) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string channel_id = info[0].As<Napi::String>().Utf8Value();
    std::string message = info[1].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[2].As<Napi::Function>();

    finalThis->httpClient->discord_send_text_message_async(channel_id, message, callback);

  }
                                                   ));
  obj.Set("editChannelMessage", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 4) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string channel_id = info[0].As<Napi::String>().Utf8Value();
    std::string message_id = info[1].As<Napi::String>().Utf8Value();
    std::string message = info[2].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[3].As<Napi::Function>();

    finalThis->httpClient->discord_edit_text_message_async(channel_id, message_id, message, callback);

  }
                                                   ));
  obj.Set("leaveGuild", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 2) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string guild_id = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    finalThis->httpClient->discord_leave_guild_async(guild_id, callback);

  }
                                           ));
  obj.Set("getGuildChannels", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 2) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string guild_id = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    finalThis->httpClient->discord_get_guild_channels_async(guild_id, callback);

  }
                                                 ));
  obj.Set("getChannelStandalone", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 2) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string channel_id = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    finalThis->httpClient->discord_get_channel_async(channel_id, callback);

  }
                                                     ));
  obj.Set("getGuildStandalone", Napi::Function::New(env, [finalThis, token](const Napi::CallbackInfo& info) {
    if (info.Length() != 2) {
      Napi::TypeError::New(info.Env(), "Wrong number of arguments")
      .ThrowAsJavaScriptException();
      return;
    }
    std::string guild_id = info[0].As<Napi::String>().Utf8Value();
    Napi::Function callback = info[1].As<Napi::Function>();

    finalThis->httpClient->discord_get_guild_async(guild_id, callback);

  }
                                                   ));

  obj.Set("connect", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {

    socket->connect();
  }
                                        ));
  return obj;
}
void Instance::generateVoiceBindings(Napi::Env env, Napi::Function callback, std::string& server_id, std::string& channel_id, DisWebsocket* sock) {
  // if(vc_socket != nullptr) {

  //   return;
  // }
  auto ref = Napi::Persistent(callback);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Audio creation callback", 0, 1);
  auto finalThis = this;
  sock->handleVoiceInit(server_id, channel_id, [sock, server_id, channel_id, tsfn, finalThis](const json& server_state, const json& voice_state) mutable {

    auto vc_socket = new DisVoiceWebsocket(server_state, voice_state, sock->own_id, server_id, channel_id);
    auto callback = [finalThis, sock]( Napi::Env env, Napi::Function jsCallback, DisVoiceWebsocket* socket) {
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
        socket->playFile(path, "mpeg");
      }
          ));
      obj.Set("playOpusFile", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
        auto env = info.Env();
        if (info.Length() != 1) {
          Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
          return;
        }
        std::string path = info[0].As<Napi::String>().Utf8Value();
        socket->playFile(path, "opus");
      }
          ));
      obj.Set("playPiped", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
        auto env = info.Env();
        if (info.Length() != 2) {
          Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
          return;
        }
        int32_t mode = info[0].As<Napi::Number>().Int32Value();
        std::string url = info[1].As<Napi::String>().Utf8Value();
        socket->playFile( url, "pipe");
      }
          ));
      obj.Set("playWavFile", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
        auto env = info.Env();
        if (info.Length() != 1) {
          Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
          return;
        }
        std::string path = info[0].As<Napi::String>().Utf8Value();
        socket->playFile(path, "wav");
      }

          ));
      obj.Set("setGain", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
        auto env = info.Env();
        if (info.Length() != 1) {
          Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
          return;
        }
        float value = info[0].As<Napi::Number>().FloatValue();
        socket->updateGain(value);
      }
          ));

      obj.Set("stop", Napi::Function::New(env, [socket](const Napi::CallbackInfo& info) {
        socket->handleStop();
      }
                                         ));
      obj.Set("disconnect", Napi::Function::New(env, [socket, finalThis, sock](const Napi::CallbackInfo& info) {
        socket->handleDisconnect();
        sock->handleVoiceLeave();
        // finalThis->voice_sock = nullptr;
      }
                                               ));
      jsCallback.Call({ obj } );
    };
    auto status = tsfn.BlockingCall( vc_socket, callback );
    if(status != napi_ok) {
      std::cout << "not ok\n";
    } else {
      finalThis->voice_connections.push_back(vc_socket);
    }
  });
}

int Instance::bootstrap() {
  discord_simple_response response = DisHttpClient::discord_get_gateway_endpoint(this->token);
  if(!response.success) {
    return 1;
  }
  std::string final_url = std::string(response.value +  "/?compress=zlib-stream&v=6&encoding=json");
  this->httpClient = new DisHttpClient(this->token);
  this->socket = new DisWebsocket(final_url, this->token, false);
  return 0;
};
