#ifndef DIS_LIGHT_VOICE_STRING_UTILS_H
#define DIS_LIGHT_VOICE_STRING_UTILS_H

#include <string>
#ifndef __APPLE__
#include <algorithm>
#endif
#ifdef _WIN32
#include <cwctype>
#include <cctype>
#endif
static inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

#endif
