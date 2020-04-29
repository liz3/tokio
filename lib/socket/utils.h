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
static discord_simple_response discord_get_gateway_endpoint(std::string& token) {
  discord_simple_response return_target;
  const char* DISCORD_API_BASE = "https://discordapp.com";
  HttpClient httpClient;
  HttpRequestArgsPtr args = httpClient.createRequest();
//  args->connectTimeout = 100000;
//  args->transferTimeout = 100000;
  WebSocketHttpHeaders headers;
  headers["Authorization"] = "Bot " + token;
  args->extraHeaders = headers;
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

#endif
