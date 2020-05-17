#ifndef DIS_LIGHT_EVENT_HANDLER_H
#define DIS_LIGHT_EVENT_HANDLER_H
#include <napi.h>
#include <string>
class DisEventListener {
 public:
  DisEventListener(Napi::ThreadSafeFunction function, std::string eventName) : function(function), evName(eventName) {}
  Napi::ThreadSafeFunction function;
  std::string evName;
};
#endif
