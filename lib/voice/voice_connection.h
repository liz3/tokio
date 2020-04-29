#ifndef DIS_LIGHT_VOICE_CONNECTION_H
#define DIS_LIGHT_VOICE_CONNECTION_H

#include "opus/opus_wrapper.h"
#include <string>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include "stringutils.h"
#include <thread>
#include <chrono>
#include <sodium.h>
#include <sodium/crypto_stream_xsalsa20.h>
constexpr auto kFrameSize = 960;
constexpr auto kNumChannels = 2;
constexpr auto kSampleRate = 48000;
class VoiceConnection {
 private:
  bool ready = false;
  bool running = true;
  opus::Encoder encoder;
  std::string& address;
  int port;
  unsigned int ssrc;
  //soxket
  int sockfd;
  struct sockaddr_in servaddr;
  int encode_seq = 0;
  int encode_count = 0;
  int timestamp = 0;
  void preparePacket(unsigned char raw[], int len);
 public:
  VoiceConnection(std::string& address, int port, int ssrc);
  static short getShortBigEndian(char arr[], int offset)
  {
    return (short) ((arr[offset    ] & 0xff) << 8
                    | arr[offset + 1] & 0xff);
  };
  static void setIntBigEndian(unsigned char arr[], int offset, int it)
    {
        arr[offset    ] = (unsigned char) ((it >> 24) & 0xFF);
        arr[offset + 1] = (unsigned char) ((it >> 16) & 0xFF);
        arr[offset + 2] = (unsigned char) ((it >> 8)  & 0xFF);
        arr[offset + 3] = (unsigned char) ( it         & 0xFF);
    }
  bool setupAndHandleSocket();
  std::string own_ip;
  int own_port = 0;
  void startHeartBeat(int interval);
  void send(char buffer[], int size);
  unsigned char* key;
  int keyLength = 0;
};

#endif
