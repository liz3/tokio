#ifndef DIS_LIGHT_VOICE_CONNECTION_H
#define DIS_LIGHT_VOICE_CONNECTION_H

#include "opus/opus_wrapper.h"
#include <string>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#include <WS2tcpip.h>
#include "win_mmap/windows-mmap.h"
#endif
#include <sstream>
#include "stringutils.h"
#include <thread>
#include <fstream>
#include <chrono>
#include <sodium.h>
#include <mad.h>
#include <opus/opusfile.h>
#ifndef __APPLE__
#include <cstring>
#endif
constexpr auto kFrameSize = 48 * 20;
constexpr auto kNumChannels = 2;
constexpr auto kSampleRate = 48000;
constexpr auto kFrameSizeSecs = 0.02;
class VoiceConnection {
 private:
  bool ready = false;
  int sendCounter;
  std::chrono::high_resolution_clock::time_point startTime;
  opus::Encoder encoder;
  std::string& address;
  int port;
  unsigned int ssrc;
  //soxket
#ifdef _WIN32
  SOCKET sockfd;
  sockaddr_in servaddr;
#else
  int sockfd;
  struct sockaddr_in servaddr;
#endif
  short encode_seq = 0;
  int encode_count = 0;
  int timestamp = 0;
  void preparePacket(uint8_t*& encodedAudioData, int len);
  //mad stuff
  struct mad_stream mad_stream;
  struct mad_frame mad_frame;
  struct mad_synth mad_synth;
  static short getShortBigEndian(char arr[], int offset) {
    return (short) ((arr[offset    ] & 0xff << 8)
                    | (arr[offset + 1] & 0xff));
  };
  static void duplicate_signal(opus_int16 * in_L,
                               opus_int16 * in_R,
                               opus_int16 * out,
                               const size_t num_samples) {
    for (size_t i = 0; i < num_samples; ++i) {
      out[i * 2] = in_L[i];
      out[i * 2 + 1] = in_R[i];
    }
  }
  static void adjustGain(float gain, std::vector<opus_int16>* values) {
    if(gain != 1) {
      for(int i = 0; i < values->size(); i++) {
        (*values)[i] = (*values)[i] * gain;
      }
    }
  }
 public:
  float gain = 1;
  VoiceConnection(std::string& address, int port, int ssrc);
  bool setupAndHandleSocket();
  std::string own_ip;
  int own_port = 0;
  void startHeartBeat(int interval);
  void send(unsigned char* buffer, int size);
  std::vector<unsigned char> key;
  int keyLength = 0;
  void playFile(std::string filePath);
  void playOpusFile(std::string filePath);
  void playWavFile(std::string filePath);
  bool interuptFlag = false;
  bool running = true;
};

#endif
