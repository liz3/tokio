#include "voice_connection.h"


VoiceConnection::VoiceConnection(std::string& address, int port, int ssrc) : encoder( opus::Encoder(kSampleRate, kNumChannels, OPUS_APPLICATION_AUDIO)), address(address), port(port), ssrc(ssrc) {
  if( sodium_init() < 0) {
    //TODO handle errror
    return;
  }

}
void VoiceConnection::startHeartBeat(int interval) {
  auto finalThis = this;
  std::thread t([interval, finalThis](){
                  while(finalThis->running) {
                    unsigned char buff[] = {(unsigned char)0xC9, 0, 0, 0, 0, 0, 0, 0, 0};
                    finalThis->send(buff, 9);
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                  }
                });
  t.detach();
}
void VoiceConnection::send(unsigned char buffer[], int size) {
  sendto(sockfd,buffer, sizeof(unsigned char) * size,
        0, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
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
  std::memcpy(nonce                , header, sizeof header);
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
  const unsigned char *input_stream =(const unsigned char*) mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);
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
  int chunkSize;  stream.read(reinterpret_cast<char *>(&chunkSize), sizeof(chunkSize));
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
  int32_t Subchunk2Size;
  stream.read(reinterpret_cast<char *>(&Subchunk2Size), sizeof(Subchunk2Size));
  char LIST[Subchunk2Size +1];
  stream.read(LIST, Subchunk2Size);
  LIST[Subchunk2Size + 1] = '\0';
  stream.read(data, 4);
  int32_t Subchunk3Size;
  stream.read(reinterpret_cast<char *>(&Subchunk3Size), sizeof(Subchunk3Size));
  sendCounter = 0;
  startTime = high_resolution_clock::now();

  while(1) {
    if(Subchunk3Size <= 0) break;
    int size = kFrameSize * 2;
    Subchunk3Size -= size * 2;
    opus_int16 buff[size];
    stream.read(reinterpret_cast<char *>(&buff), size * 2);
    std::vector<opus_int16> values(buff, buff + size);
    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(values, kFrameSize);
    for(auto entry : opus_out) {
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
    std::vector<opus_int16> values(buff, buff + size);
    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(values, kFrameSize);
    for(auto entry : opus_out) {
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
  sendto(sockfd, (const char *)buf.str().c_str(), 70,
        MSG_WAITALL, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
  int n;
  socklen_t len;
  char recv_buff[1024];
  n = recvfrom(sockfd, (char *)recv_buff, 1024,MSG_WAITALL, (struct sockaddr *) &servaddr,
                &len);
  recv_buff[n] = '\0';
  std::string ip = std::string(recv_buff, 4, n -6);
  trim(ip);
  int port = (int) VoiceConnection::getShortBigEndian(recv_buff, n - 2) & 0xFFFF;
  std::string out = std::string(recv_buff);
  own_ip = ip;
  own_port = port;
  return true;
}
