#ifndef DIS_LIGHT_DIS_VOICE_WEBSOCKET_H
#define DIS_LIGHT_DIS_VOICE_WEBSOCKET_H

#include <ixwebsocket/IXWebSocket.h>
#include <iostream>
#include <napi.h>
#include <nlohmann/json.hpp>
#include <thread>         // std::thread
#include <chrono>
#include <string>
#include <unistd.h>
#include <optional>
#include "zlib.h"
#include "zconf.h"
#include <sstream>
#include "discord_utils.h"
#include "../zstr/zstr.h"
#include "../voice/voice_connection.h"
#include "event_handler.h"
typedef void (* vCallback)(Napi::Env env, Napi::Function jsCallback, std::string* value);
using json = nlohmann::json;
class DisVoiceWebsocket {
public:
  DisVoiceWebsocket(const json& server_state, const json& voice_state, std::string& id, std::string& server_id, std::string& channel_id);
  void connect();
  void stop();
  void externalClose(const Napi::CallbackInfo& info);
  void registerEventListener(std::string& name, Napi::Function callback);
  VoiceConnection* voiceConn = nullptr;
  void playFile(std::string& path, std::string type);
  void handleStop();
  void handleDisconnect();
private:
  std::vector<DisEventListener> event_handlers;
  bool running = false;
  ix::WebSocket webSocket;
  void messageHandler(int messageOpCode, json data, const std::string& msg);
  void close();
  void sendMessage(int opcode, json data);
  void sendHeartBeat();
  static void setupHeartBeatInterval(DisVoiceWebsocket* instance, int interval);
  void handleAuth();
  void updateSpeakingState(bool speaking);
  long seq;
  std::string token;
  std::stringstream message_stream;
  unsigned int ZLIB_SUFFIX = 0x0000FFFF;
  std::unique_ptr<zstr::istream> zlib_ctx;
  std::string ip;
  std::string session_id;
  void resume();
  bool resumed = false;
  int ssrc = 0;
  int port = 0;
  bool connected = false;
  std::string dis_id;
  std::string server_id;
  std::string channel_id;
  int heart_beat_interval = 0;
  bool playing = false;

};
#endif
