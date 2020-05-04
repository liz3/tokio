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
#include "event_handler.h"
using json = nlohmann::json;
typedef void (* vCallback)(Napi::Env env, Napi::Function jsCallback, std::string* value);
using VoiceInitCallback = std::function<void(const json&,const json&)>;
class DisWebsocket {
public:
  DisWebsocket(std::string& socket_url, std::string& token, bool auto_connect);
  void connect();
  void stop();
  void externalClose(const Napi::CallbackInfo& info);
  void registerEventListener(std::string& name, Napi::Function callback);
  void handleVoiceInit(std::string& server_id, std::string& channel_id, const VoiceInitCallback& cb);
  std::string own_id;
  void handleVoiceLeave();
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
  bool waitingVoiceConnect = false;
  VoiceInitCallback voiceInitCallback;
  std::vector<json> voice_states;

};
#endif
