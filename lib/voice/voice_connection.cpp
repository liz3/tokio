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
  encode_seq++;
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
  timestamp += kFrameSize;

}
void VoiceConnection::playFile(std::string filePath) {
  mad_stream_init(&mad_stream);
  mad_synth_init(&mad_synth);
  mad_frame_init(&mad_frame);
  std::vector<opus_int16> audio_set;
  const char* filename = filePath.c_str();
  FILE *fp = fopen(filename, "r");
  int fd = fileno(fp);
  struct stat metadata;
  if (fstat(fd, &metadata) >= 0) {
//    printf("File size %d bytes\n", (int)metadata.st_size);
  } else {
    printf("Failed to stat %s\n", filename);
    fclose(fp);
    return;
  }
  const unsigned char *input_stream =(const unsigned char*) mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);
  mad_stream_buffer(&mad_stream, input_stream, metadata.st_size);
  int s = kFrameSize;
  const int extraBuffer = 100;
  int sendTime = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
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

        begin = std::chrono::steady_clock::now();

        std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(audio_set, kFrameSize);
        auto entry = opus_out[0];
        uint8_t * encodedAudioDataPointer = &entry[0];
        this->preparePacket(encodedAudioDataPointer, entry.size());

        end = std::chrono::steady_clock::now();
        sendTime = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

        #ifdef __APPLE__
        std::this_thread::sleep_for(std::chrono::microseconds(17500-sendTime-extraBuffer));
        #else
        std::this_thread::sleep_for(std::chrono::microseconds(20000-sendTime-extraBuffer));
        #endif

        s = kFrameSize;
        audio_set.clear();
      }
    }
  }

  mad_synth_finish(&mad_synth);
  mad_frame_finish(&mad_frame);
  mad_stream_finish(&mad_stream);
  fclose(fp);
}
void VoiceConnection::playOpusFile(std::string filePath) {

  const char* filename = filePath.c_str();
  int* opus_err;
  OggOpusFile *file = op_open_file(filename, opus_err);
  const int extraBuffer = 100;
  int sendTime = 0;
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  while(1) {
    if(this->interuptFlag) {
      this->running = false;
      this->interuptFlag = false;
      op_free(file);
      return;
    }
    int size = 960 * 2;
    opus_int16* buff = new opus_int16[size];
    int o = op_read(file,buff,size, NULL);
    if(o <= 0) break;
    begin = std::chrono::steady_clock::now();
    std::vector<opus_int16> values(buff, buff + size);
    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(values, kFrameSize);
    for(auto entry : opus_out) {
      uint8_t * encodedAudioDataPointer = &entry[0];
      this->preparePacket(encodedAudioDataPointer, entry.size());

      end = std::chrono::steady_clock::now();
      sendTime = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

#ifdef __APPLE__
      std::this_thread::sleep_for(std::chrono::microseconds(17600-sendTime-extraBuffer));
#else
      std::this_thread::sleep_for(std::chrono::microseconds(20000-sendTime-extraBuffer));
#endif

    }
    delete[] buff;
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
