#ifndef DIS_LIGHT_SOCKET_UTILS_H
#define DIS_LIGHT_SOCKET_UTILS_H
#include <nlohmann/json.hpp>
#include <ixwebsocket/IXHttpClient.h>
using json = nlohmann::json;
using namespace ix;
struct discord_simple_response {
  bool success;
  std::string value;
};
static WebSocketHttpHeaders get_default_headers(std::string& token) {
  WebSocketHttpHeaders headers;
  headers["Authorization"] = "Bot " + token;
  headers["Content-Type"] = "application/json";
  headers["User-Agent"] = "dis-light/1.0";
  return headers;
}
static discord_simple_response discord_get_gateway_endpoint(std::string& token) {
  discord_simple_response return_target;
  const char* DISCORD_API_BASE = "https://discordapp.com";
  HttpClient httpClient;
  HttpRequestArgsPtr args = httpClient.createRequest();
  args->extraHeaders = get_default_headers(token);
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
static void discord_handle_reply(const HttpResponsePtr& out, Napi::ThreadSafeFunction tsfn) {
  auto callback = []( Napi::Env env, Napi::Function jsCallback,
                      discord_simple_response *resp) {
                                                      jsCallback.Call({Napi::Boolean::New(env, resp->success),Napi::String::New(env, resp->value)});
  };
  discord_simple_response* return_target = new discord_simple_response();
  auto errorCode = out->errorCode;
  if(errorCode != HttpErrorCode::Ok) {
    return_target->value = out->errorMsg;
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
static void discord_send_text_message_async(std::string token, std::string& channel, std::string& body, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/channels/" + channel + "/messages";
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Message Create Callback", 0, 1);
  HttpClient* httpClient = new HttpClient(true);
  auto args = httpClient->createRequest(url, HttpClient::kPost);
  args->extraHeaders = get_default_headers(token);
  args->body = body;
  bool ok = httpClient->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                            {
                                              discord_handle_reply(out, tsfn);
                                            });
}
static void discord_edit_text_message_async(std::string token, std::string& channel, std::string& message_id, std::string& body, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/channels/" + channel + "/messages/" + message_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Message Edit Callback", 0, 1);
  HttpClient* httpClient = new HttpClient(true);
  auto args = httpClient->createRequest(url, HttpClient::kPatch);
  args->extraHeaders = get_default_headers(token);
  args->body = body;
  bool ok = httpClient->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                            {
                                              discord_handle_reply(out, tsfn);
                                            });
}

static void discord_leave_guild(std::string token, std::string& guild_id, Napi::Function cbFunc) {
  const char* DISCORD_API_BASE = "https://discordapp.com";
  std::string url = std::string(DISCORD_API_BASE) + "/api/users/@me/guilds/" + guild_id;
  auto ref = Napi::Persistent(cbFunc);
  auto tsfn = Napi::ThreadSafeFunction::New(ref.Env(),ref.Value(),"Guild Leave Callback", 0, 1);
  HttpClient* httpClient = new HttpClient(true);
  auto args = httpClient->createRequest(url, HttpClient::kDel);
  args->extraHeaders = get_default_headers(token);
  bool ok = httpClient->performRequest(args, [tsfn](const HttpResponsePtr& out)
                                       {
                                         discord_handle_reply(out, tsfn);

    });
}
#endif
