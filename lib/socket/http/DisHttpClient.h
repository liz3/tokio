#ifndef DIS_LIGHT_DIS_HTTPCLIENT_H
#define DIS_LIGHT_DIS_HTTPCLIENT_H
#include<string>
#include <ixwebsocket/IXHttpClient.h>
#include <napi.h>
#include <nlohmann/json.hpp>
#include <iostream>
#ifdef _WIN32
#include <cstdlib>
#endif
struct discord_simple_response {
  bool success;
  std::string value;
};

class DisHttpClient {
 public:
  DisHttpClient(std::string& token);
  static discord_simple_response discord_get_gateway_endpoint(std::string& token);
  void discord_send_text_message_async(std::string& channel, std::string& body, Napi::Function cbFunc);
  void discord_edit_text_message_async(std::string& channel, std::string& message_id, std::string& body, Napi::Function cbFunc);
  void discord_leave_guild_async(std::string& guild_id, Napi::Function cbFunc);
  void discord_get_guild_channels_async(std::string& guild_id, Napi::Function cbFunc);
  void discord_get_channel_async(std::string& channel_id, Napi::Function cbFunc);
  void discord_get_guild_async(std::string& guild_id, Napi::Function cbFunc);
 private:
  static ix::WebSocketHttpHeaders get_default_headers(std::string& token) {
    ix::WebSocketHttpHeaders headers;
    headers["Authorization"] = "Bot " + token;
    headers["Content-Type"] = "application/json";
    headers["User-Agent"] = "dis-light/1.0";
    return headers;
  }
  std::string token;
  ix::HttpClient* client = nullptr;
  static void discord_handle_reply(const ix::HttpResponsePtr& out, Napi::ThreadSafeFunction tsfn);
};
#endif
