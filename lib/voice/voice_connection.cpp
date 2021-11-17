#include "voice_connection.h"
#include <signal.h>


VoiceConnection::VoiceConnection(std::string& address, int port, int ssrc) : encoder( opus::Encoder(kSampleRate, kNumChannels, OPUS_APPLICATION_AUDIO)), address(address), port(port), ssrc(ssrc) {
  if( sodium_init() < 0) {
    //TODO handle errror
    return;
  }

}
void VoiceConnection::playPiped(int mode, std::string url) {
  using namespace std::chrono;
  int fd[2], pid;
  pipe(fd);
  pid = fork();
  if(pid == 0) {

    close (1);
    dup(fd[1]);
    close (0);
    close (2);
    close (fd[0]);
    close (fd[1]);
    std::vector<std::string> args;
    args.push_back("ffmpeg");
    args.push_back("-user_agent");
    args.push_back("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.114 Safari/537.36");
    args.push_back("-i");
    args.push_back(url);
    args.push_back("-f");
    args.push_back("data");
    args.push_back("-map");
    args.push_back("0:a");
    args.push_back("-ar");
    args.push_back("48k");
    args.push_back("-ac");
    args.push_back("2");
    args.push_back("-acodec");
    args.push_back("pcm_s16le");
    args.push_back("-sample_fmt");
    args.push_back("s16");
    args.push_back("-vbr");
    args.push_back("off");
    args.push_back("-b:a");
    args.push_back("64000");
    args.push_back("pipe:1");
    const char** args_arr = new const char* [args.size() + 1];
    for(size_t i = 0; i < args.size(); i++) {
      args_arr[i] = args[i].c_str();
    }
    args_arr[args.size()] = nullptr;
    execvp(args_arr[0], static_cast<char* const*>((void*)args_arr));

  } else {

    sendCounter = 0;
    close (fd[1]);
     std::vector<std::vector<opus_int16>> cached_frames;
     auto finalThis = this;
     std::thread t([&cached_frames, &fd, &finalThis](){
       size_t needed = (kFrameSize * 2);
       std::vector<opus_int16> data_buffer;
       int last = 1;
         while(last != 0 && !finalThis->interuptFlag) {
           int remaining = needed - data_buffer.size();
           opus_int16* buf = new opus_int16[remaining];
           int received = read(fd[0], buf, sizeof(opus_int16) * remaining) / sizeof(opus_int16);
           last = received;
           auto decoded = std::vector<opus_int16>(buf, buf + received);
           if(decoded.size() > remaining) {
             data_buffer.insert(data_buffer.end(), decoded.begin(), decoded.begin()+remaining);
             cached_frames.push_back(data_buffer);
             std::vector<opus_int16> next(decoded.begin()+remaining, decoded.end());
             data_buffer=next;
           } else {
             data_buffer.insert(data_buffer.end(), decoded.begin(), decoded.end());
             if(data_buffer.size() == needed) {
               cached_frames.push_back(data_buffer);
               data_buffer.clear();
             }
           }
       }
         close(fd[0]);
     });
     while(true) {

       if(this->interuptFlag) {
         kill(pid, SIGTERM);
         wait(&pid);
         this->running = false;
         this->interuptFlag = false;
         t.join();
         return;
       }

       if (cached_frames.size() == 160) {
         startTime = high_resolution_clock::now();
       }
       if (cached_frames.size() >= 160) {
         auto current_frame = cached_frames[sendCounter];
         adjustGain(gain, &current_frame);
         std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(current_frame, kFrameSize);
         for(auto entry : opus_out) {
           uint8_t * encodedAudioDataPointer = &entry[0];
           this->preparePacket(encodedAudioDataPointer, entry.size());
         }

         ++sendCounter;
         if(sendCounter >= cached_frames.size())
           break;

         high_resolution_clock::time_point t2 = high_resolution_clock::now();
         duration<double> abs_elapsed = duration_cast<duration<double>>(t2 - startTime);
         double play_head = kFrameSizeSecs * sendCounter;
         double delay = play_head - abs_elapsed.count();
         if(delay > 0)
           std::this_thread::sleep_for(std::chrono::microseconds((int) (delay * 1000000)));

       }
     }
     t.join();
  }
}
void VoiceConnection::startHeartBeat(int interval) {
  auto finalThis = this;
  std::thread t([interval, finalThis]() {
    while(finalThis->running) {
      unsigned char buff[] = {(unsigned char)0xC9, 0, 0, 0, 0, 0, 0, 0, 0};
      finalThis->send(buff, 9);
      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
    }
  });
  t.detach();
}
void VoiceConnection::send(unsigned char* buffer, int size) {
#ifdef _WIN32
  int result = sendto(sockfd,reinterpret_cast<const char*>(buffer), sizeof(const char) * size,
                      0, ( SOCKADDR *) &servaddr,
                      sizeof(servaddr));
  if (result == SOCKET_ERROR) {
    std::cout << "send failed: " << WSAGetLastError() << "\n";
  }
#else
  sendto(sockfd,buffer, sizeof(unsigned char) * size,
         0, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
#endif
}
void VoiceConnection::preparePacket(uint8_t*& encodedAudioData, int len) {
  const uint8_t header[12] = {
    0x80,
    0x78,
    static_cast<uint8_t>((encode_seq  >> (8 * 1)) & 0xff),
    static_cast<uint8_t>((encode_seq  >> (8 * 0)) & 0xff),
    static_cast<uint8_t>((timestamp >> (8 * 3)) & 0xff),
    static_cast<uint8_t>((timestamp >> (8 * 2)) & 0xff),
    static_cast<uint8_t>((timestamp >> (8 * 1)) & 0xff),
    static_cast<uint8_t>((timestamp >> (8 * 0)) & 0xff),
    static_cast<uint8_t>((ssrc      >> (8 * 3)) & 0xff),
    static_cast<uint8_t>((ssrc      >> (8 * 2)) & 0xff),
    static_cast<uint8_t>((ssrc      >> (8 * 1)) & 0xff),
    static_cast<uint8_t>((ssrc      >> (8 * 0)) & 0xff),
  };
  uint8_t nonce[24];
  std::memcpy(nonce, header, sizeof header);
  std::memset(nonce + sizeof header,      0, sizeof nonce - sizeof header);
  std::vector<uint8_t> audioDataPacket(sizeof header + len + crypto_secretbox_MACBYTES);
  std::memcpy(audioDataPacket.data(), header, sizeof header);
  crypto_secretbox_easy(audioDataPacket.data() + sizeof header,
                        encodedAudioData, len, nonce, &key[0]);
  this->send(audioDataPacket.data(), audioDataPacket.size());
  encode_seq++;
  timestamp += kFrameSize;
}
void VoiceConnection::playFile(std::string filePath) {

  using namespace std::chrono;
  mad_stream_init(&mad_stream);
  mad_synth_init(&mad_synth);
  mad_frame_init(&mad_frame);
  std::vector<opus_int16> audio_set;
  const char* filename = filePath.c_str();
  FILE *fp = fopen(filename, "r");
  int fd = fileno(fp);
  sendCounter = 0;
  startTime = high_resolution_clock::now();
  struct stat metadata;
  if (fstat(fd, &metadata) >= 0) {

  } else {
    printf("Failed to stat %s\n", filename);
    fclose(fp);
    return;
  }
#ifdef _WIN32
  const unsigned char *input_stream =(const unsigned char*) mmap(0, metadata.st_size, PROT_READ, 0x02, fd, 0);
#else
  const unsigned char *input_stream =(const unsigned char*) mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);
#endif
  mad_stream_buffer(&mad_stream, input_stream, metadata.st_size);
  int s = kFrameSize;
  while (1) {
    if(this->interuptFlag) {
      this->running = false;
      this->interuptFlag = false;
      mad_synth_finish(&mad_synth);
      mad_frame_finish(&mad_frame);
      mad_stream_finish(&mad_stream);
      fclose(fp);
      return;
    }
    if (mad_frame_decode(&mad_frame, &mad_stream)) {
      if (MAD_RECOVERABLE(mad_stream.error)) {
        continue;
      } else if (mad_stream.error == MAD_ERROR_BUFLEN) {
        break;
      } else {
        break;
      }
    }

    mad_synth_frame(&mad_synth, &mad_frame);
    auto pcm = mad_synth.pcm;
    if (pcm.channels != 2) {
      std::cout << "not two channels, returning\n";
      return;
    }

    int sample_len = mad_synth.pcm.length;
    mad_fixed_t const *left_ch = pcm.samples[0], *right_ch = pcm.samples[1];

    while(sample_len--) {
      int left;
      int right;
      left = (*left_ch++);
      right = (*right_ch++);
      opus_int16 l,r;
      l = (opus_int16)(left >> 16);
      r = (opus_int16)(right >> 16);
      audio_set.push_back(l);
      audio_set.push_back(r);
      --s;
      if(s == 0 || sample_len<0) {
        adjustGain(gain, &audio_set);
        std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(audio_set, kFrameSize);
        auto entry = opus_out[0];
        uint8_t * encodedAudioDataPointer = &entry[0];
        this->preparePacket(encodedAudioDataPointer, entry.size());

        s = kFrameSize;
        audio_set.clear();
        //sleep logic
        ++sendCounter;

        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> abs_elapsed = duration_cast<duration<double>>(t2 - startTime);
        double play_head = kFrameSizeSecs * sendCounter;
        double delay = play_head - abs_elapsed.count();
        if(delay > 0)
          std::this_thread::sleep_for(std::chrono::microseconds((int) (delay * 1000000)));

      }
    }
  }

  mad_synth_finish(&mad_synth);
  mad_frame_finish(&mad_frame);
  mad_stream_finish(&mad_stream);
  fclose(fp);

}
void VoiceConnection::playWavFile(std::string filePath) {
  using namespace std::chrono;
  std::ifstream stream(filePath.c_str());
  char header [5];
  stream.read(header, 4);
  header[4] = '\0';
  int chunkSize;
  stream.read(reinterpret_cast<char *>(&chunkSize), sizeof(chunkSize));
  int rawAudioSize = chunkSize - 36;
  char wave[5];
  stream.read(wave, 4);
  wave[4] = '\0';
  char fmt[5];
  stream.read(fmt, 4);
  fmt[4] = '\0';
  int Subchunk1Size;
  stream.read(reinterpret_cast<char *>(&Subchunk1Size), sizeof(Subchunk1Size));
  int16_t AudioFormat;
  stream.read(reinterpret_cast<char *>(&AudioFormat), sizeof(AudioFormat));
  int16_t NumChannels;
  stream.read(reinterpret_cast<char *>(&NumChannels), sizeof(NumChannels));
  int32_t SampleRate;
  stream.read(reinterpret_cast<char *>(&SampleRate), sizeof(SampleRate));
  int32_t ByteRate;
  stream.read(reinterpret_cast<char *>(&ByteRate), sizeof(ByteRate));
  int16_t BlockAlign;
  stream.read(reinterpret_cast<char *>(&BlockAlign), sizeof(BlockAlign));
  int16_t BitsPerSample;
  stream.read(reinterpret_cast<char *>(&BitsPerSample), sizeof(BitsPerSample));
  char data [5];
  stream.read(data, 4);
  data[4] = '\0';
  std::string asStr = std::string(data);
  if(asStr == "LIST") {
    int32_t Subchunk2Size;
    stream.read(reinterpret_cast<char *>(&Subchunk2Size), sizeof(Subchunk2Size));
    int length = stream.tellg();
    stream.seekg(length + Subchunk2Size + 4);
  }
  int32_t Subchunk3Size;
  stream.read(reinterpret_cast<char *>(&Subchunk3Size), sizeof(Subchunk3Size));
  sendCounter = 0;
  int size = kFrameSize * 2;
  startTime = high_resolution_clock::now();
  while(1) {
    if(Subchunk3Size <= 0) break;
    if(this->interuptFlag) {
      this->running = false;
      this->interuptFlag = false;
      stream.close();
      return;
    }
    opus_int16* buff = new opus_int16[size];
    if(NumChannels == 1) {
      opus_int16* actBuff = new opus_int16[kFrameSize];
      stream.read(reinterpret_cast<char *>(actBuff), kFrameSize * 2);
      duplicate_signal(actBuff,actBuff,buff,kFrameSize);
      delete [] actBuff;
      Subchunk3Size -= kFrameSize * 2;
    } else {
      Subchunk3Size -= size * 2;
      stream.read(reinterpret_cast<char *>(buff), size * 2);
    }
    std::vector<opus_int16> values(buff, buff + size);
    adjustGain(gain, &values);
    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(values, kFrameSize);
    for(auto entry : opus_out) {
      uint8_t * encodedAudioDataPointer = &entry[0];
      this->preparePacket(encodedAudioDataPointer, entry.size());
    }
    delete[] buff;
    //sleep logic
    ++sendCounter;

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> abs_elapsed = duration_cast<duration<double>>(t2 - startTime);
    double play_head = kFrameSizeSecs * sendCounter;
    double delay = play_head - abs_elapsed.count();
    if(delay > 0)
      std::this_thread::sleep_for(std::chrono::microseconds((int) (delay * 1000000)));

  }
  stream.close();
}
void VoiceConnection::playOpusFile(std::string filePath) {
  using namespace std::chrono;
  const char* filename = filePath.c_str();
  int* opus_err;
  OggOpusFile *file = op_open_file(filename, opus_err);
  sendCounter = 0;
  startTime = high_resolution_clock::now();
  while(1) {
    if(this->interuptFlag) {
      this->running = false;
      this->interuptFlag = false;
      op_free(file);
      return;
    }
    int size = kFrameSize * 2;
    opus_int16* buff = new opus_int16[size];
    int o = op_read(file,buff,size, NULL);
    if(o <= 0) break;
    std::cout << o  << "\n";
    std::vector<opus_int16> values(buff, buff + size);
    adjustGain(gain, &values);
    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(values, kFrameSize);
    std::cout << "length" << opus_out.size() << "\n";
    for(auto entry : opus_out) {
      std::cout << entry.size() << "\n";
      uint8_t * encodedAudioDataPointer = &entry[0];
      this->preparePacket(encodedAudioDataPointer, entry.size());
    }
    //sleep logic
    ++sendCounter;

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> abs_elapsed = duration_cast<duration<double>>(t2 - startTime);
    double play_head = kFrameSizeSecs * sendCounter;
    double delay = play_head - abs_elapsed.count();
    if(delay > 0)
      std::this_thread::sleep_for(std::chrono::microseconds((int) (delay * 1000000)));
  }
  op_free(file);
}
bool VoiceConnection::setupAndHandleSocket() {
  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  servaddr.sin_addr.s_addr = inet_addr(address.c_str());
  std::stringstream buf;
  buf << 1;
  buf<< 70;
  buf << ssrc;
  for(int i = 0; i < 65; i++) {
    buf << '0';
  }
  buf.flush();
#ifdef _WIN32
  sendto(sockfd, (const char *)buf.str().c_str(), 70,
         0, ( SOCKADDR *) &servaddr,
         sizeof(servaddr));
#else
  sendto(sockfd, (const char *)buf.str().c_str(), 70,
         MSG_WAITALL, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
#endif
  int n;
  socklen_t len;
  char recv_buff[1024];
#ifdef _WIN32
  n = recv(sockfd, (char *)recv_buff, 1024, 0);
#else
  n = recvfrom(sockfd, (char *)recv_buff, 1024,MSG_WAITALL, (struct sockaddr *) &servaddr,
               &len);
#endif
  recv_buff[n] = '\0';

  std::string ip = std::string(recv_buff, 4, n -6);
  trim(ip);
  int port = (int) VoiceConnection::getShortBigEndian(recv_buff, n - 2) & 0xFFFF;
  own_ip = ip;
  own_port = port;
  return true;
}
