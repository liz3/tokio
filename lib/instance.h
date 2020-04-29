#ifndef DIS_LIGHT_INSTANCE_H
#define DIS_LIGHT_INSTANCE_H

#include <iostream>
#include <string>
#include <napi.h>
#include "socket/utils.h"
#include "socket/dis_websocket.h"
class Instance {
public:
  Instance(std::string token);
  Napi::Object generateBindings(Napi::Env env);
  int bootstrap();
  bool hold = true;
  DisWebsocket* socket = nullptr;
private:
  std::string token;

};
#endif //DIS_LIGHT_INSTANCE_H
