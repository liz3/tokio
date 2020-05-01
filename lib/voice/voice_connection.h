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
#include <mad.h>
#include <sys/stat.h>
#include <sys/mman.h>
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
  std::string preparePacket(unsigned char raw[], int len);
  //mad stuff
  struct mad_stream mad_stream;
  struct mad_frame mad_frame;
  struct mad_synth mad_synth;

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
 static int scale(mad_fixed_t sample) {
     /* round */
     sample += (1L << (MAD_F_FRACBITS - 16));
     /* clip */
     if (sample >= MAD_F_ONE)
         sample = MAD_F_ONE - 1;
     else if (sample < -MAD_F_ONE)
         sample = -MAD_F_ONE;
     /* quantize */
     return sample >> (MAD_F_FRACBITS + 1 - 16);
}
  bool setupAndHandleSocket();
  std::string own_ip;
  int own_port = 0;
  void startHeartBeat(int interval);
  void send(unsigned char buffer[], int size);
  unsigned char* key;
  int keyLength = 0;
  void play_test();

};

#endif