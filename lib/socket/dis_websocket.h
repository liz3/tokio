#ifndef DIS_LIGHT_DIS_WEBSOCKET_H
#define DIS_LIGHT_DIS_WEBSOCKET_H

#include <ixwebsocket/IXWebSocket.h>
#include <iostream>
#include <napi.h>
#include <nlohmann/json.hpp>
#include <thread>         // std::thread
#include <chrono>
#include <string>
#include <optional>
#include "zlib.h"
#include "zconf.h"
#include <sstream>
#include "discord_utils.h"
#include "../zstr/zstr.h"
typedef void (* vCallback)(Napi::Env env, Napi::Function jsCallback, std::string* value);
using json = nlohmann::json;
class DisEventListener {
public:
  DisEventListener(Napi::ThreadSafeFunction function, std::string eventName) : function(function), evName(eventName){}
  Napi::ThreadSafeFunction function;
  std::string evName;
};
class DisWebsocket {
public:
  DisWebsocket(std::string& socket_url, std::string& token, bool auto_connect);
  void connect();
  void stop();
  void externalClose(const Napi::CallbackInfo& info);
  void registerEventListener(std::string& name, Napi::Function callback);
private:
  std::vector<DisEventListener> event_handlers;
  bool running = false;
  ix::WebSocket webSocket;
  void messageHandler(const std::string& msg);
  void close();
  void sendMessage(int opcode, json data);
  void sendHeartBeat();
  static void setupHeartBeatInterval(DisWebsocket* instance, int interval);
  void handleAuth();
  long seq;
  std::string token;
  std::stringstream message_stream;
  unsigned int ZLIB_SUFFIX = 0x0000FFFF;
  std::unique_ptr<zstr::istream> zlib_ctx;
  static std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}
  static unsigned int discord_packet_suffix(const char* sink, int offset) {
  return ((unsigned char)sink[offset + 3] & 0xFF)
        | ((unsigned char)sink[offset + 2] & 0xFF) << 8
                | ((unsigned char)sink[offset + 1] & 0xFF) << 16
                | ((unsigned char)sink[offset] & 0xFF) << 24;
}


};
#endif
