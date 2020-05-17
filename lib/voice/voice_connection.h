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

 public:
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

  VoiceConnection(std::string& address, int port, int ssrc);
  static short getShortBigEndian(char arr[], int offset)
  {
    return (short) ((arr[offset    ] & 0xff << 8)
                    | (arr[offset + 1] & 0xff));
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
static std::vector<unsigned char> to_vector(std::stringstream& ss)
{
    // discover size of data in stream
    ss.seekg(0, std::ios::beg);
    auto bof = ss.tellg();
    ss.seekg(0, std::ios::end);
    auto stream_size = std::size_t(ss.tellg() - bof);
    ss.seekg(0, std::ios::beg);

    // make your vector long enough
    std::vector<unsigned char> v(stream_size);

    // read directly in
    ss.read((char*)v.data(), std::streamsize(v.size()));

    return v;
}


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
