#include "DisHttpClient.h"
using namespace ix;
using json = nlohmann::json;
DisHttpClient::DisHttpClient(std::string& token) {
  this->token = token;
  this->client = new HttpClient(true);
  #ifdef _WIN32 
  ix::SocketTLSOptions opt;
  opt.caFile = std::string(std::getenv("TOKIO_CERTPATH"));
  this->client->setTLSOptions(opt);
  #endif
}
void DisHttpClient::discord_handle_reply(const HttpResponsePtr& out, Napi::ThreadSafeFunction tsfn) {
    auto callback = []( Napi::Env env, Napi::Function jsCallback,
                      discord_simple_response *resp) {
                                                      jsCallback.Call({Napi::Boolean::New(env, resp->success),Napi::String::New(env, resp->value)});
  };
  discord_simple_response* return_target = new discord_simple_response();
  auto errorCode = out->errorCode;
  auto responseCode = out->statusCode;
  if(errorCode != HttpErrorCode::Ok || responseCode > 299 || responseCode < 200) {
    if(errorCode != HttpErrorCode::Ok) {
      json j;
      j["message"] = out->errorMsg;
      return_target->value = j.dump();
    }else {
      return_target->value = out->payload;
}
    return_target->success = false;
  } else {
    auto payload = out->payload;
    return_target->value = payload;
    return_target->success = true;
  }
  auto status = tsfn.BlockingCall(return_target, callback);
  if(status != napi_ok) {
    std::cout << "not ok\n";
  }

}
discord_simple_response DisHttpClient::discord_get_gateway_endpoint(std::string& token) {
  discord_simple_response return_target;
  const char* DISCORD_API_BASE = "https://discordapp.com";
  HttpClient httpClient;
  HttpRequestArgsPtr args = httpClient.createRequest();
  #ifdef _WIN32 
  args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  ix::SocketTLSOptions opt;
  opt.caFile = std::string(std::getenv("TOKIO_CERTPATH"));
  httpClient.setTLSOptions(opt);
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(token);
  HttpResponsePtr out;
  std::string url = std::string(DISCORD_API_BASE) + "/api/gateway/bot";
  out = httpClient.get(url, args);
  auto errorCode = out->errorCode;
  if(errorCode != HttpErrorCode::Ok) {
    return_target.success = false;
    return_target.value = std::string(out->errorMsg);
    return return_target;
  }
  auto payload = out->payload;
  auto parsed = json::parse(std::string(payload));
  return_target.success = true;
  return_target.value = parsed["url"];
  return return_target;
}
void DisHttpClient::discord_send_text_message_async(std::string& channel, std::string& body, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/channels/" + channel + "/messages";
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Message Create Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kPost);
    #ifdef _WIN32 
    args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  args->body = body;
  client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                            {
                                              DisHttpClient::discord_handle_reply(out, tsfn);
                                            });
}
void DisHttpClient::discord_edit_text_message_async(std::string& channel, std::string& message_id, std::string& body, Napi::Function cbFunc) {
 #ifndef _WIN32

  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/channels/" + channel + "/messages/" + message_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Message Edit Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kPatch);
  
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  args->body = body;
  client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                            {
                                              DisHttpClient::discord_handle_reply(out, tsfn);
                                            });

#endif
}
void DisHttpClient::discord_leave_guild_async(std::string& guild_id, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/users/@me/guilds/" + guild_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Guild Leave Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kDel);
    #ifdef _WIN32 
    args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                               {
                                         DisHttpClient::discord_handle_reply(out, tsfn);

                               });

}
void DisHttpClient::discord_get_guild_channels_async(std::string& guild_id, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/guilds/" + guild_id + "/channels";
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Guild ChannelGet Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kGet);
    #ifdef _WIN32 
    args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                       {
                                         DisHttpClient::discord_handle_reply(out, tsfn);

                                       });

}
void DisHttpClient::discord_get_channel_async(std::string& channel_id, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/channels/" + channel_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"ChannelGet Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kGet);
    #ifdef _WIN32 
    args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  bool ok = client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                       {
                                         DisHttpClient::discord_handle_reply(out, tsfn);

                                       });

}
void DisHttpClient::discord_get_guild_async(std::string& guild_id, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/guilds/" + guild_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"GuildGet Callback", 0, 1);
  auto args = client->createRequest(url, HttpClient::kGet);
    #ifdef _WIN32 
    args->connectTimeout = 10000;
  args->transferTimeout = 10000;
  #endif
  args->extraHeaders = DisHttpClient::get_default_headers(this->token);
  bool ok = client->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                             {
                                               DisHttpClient::discord_handle_reply(out, tsfn);

    });

}
