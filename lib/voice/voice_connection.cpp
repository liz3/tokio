#include "voice_connection.h"


VoiceConnection::VoiceConnection(std::string& address, int port, int ssrc) : encoder( opus::Encoder(kSampleRate, kNumChannels, OPUS_APPLICATION_AUDIO)), address(address), port(port), ssrc(ssrc) {

//  this->setupAndHandleSocket();
}
void VoiceConnection::startHeartBeat(int interval) {
  auto finalThis = this;
  std::thread t([interval, finalThis](){
                  std::cout << "in keepalive thread\n";
                  while(finalThis->running) {
                    std::cout << "sending keep alive\n";

                    unsigned char buff[] = {(unsigned char)0xC9, 0, 0, 0, 0, 0, 0, 0, 0};
                    finalThis->send(buff, 9);
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                  }
                });
  t.detach();
}
void VoiceConnection::send(unsigned char buffer[], int size) {
  std::cout << address << ":" << port << "\n";
  sendto(sockfd,buffer, sizeof(unsigned char) * size,
        MSG_WAITALL, ( struct sockaddr *) &servaddr,
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
void VoiceConnection::play_test() {
//  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  mad_stream_init(&mad_stream);
  mad_synth_init(&mad_synth);
  mad_frame_init(&mad_frame);
  std::vector<opus_int16> audio_set;
  std::vector<opus_int16> next_frame;

  //TEMP!!
  auto path = std::string("/Users/liz3/Downloads/FILV x Beatmount - Say What You Wanna.mp3");
  const char* filename = path.c_str();



  FILE *fp = fopen(filename, "r");
  int fd = fileno(fp);

  // Fetch file size, etc
  struct stat metadata;
  if (fstat(fd, &metadata) >= 0) {
    printf("File size %d bytes\n", (int)metadata.st_size);
  } else {
    printf("Failed to stat %s\n", filename);
    fclose(fp);
    return;
  }
  const unsigned char *input_stream =(const unsigned char*) mmap(0, metadata.st_size, PROT_READ, MAP_SHARED, fd, 0);

  // Copy pointer and length to mad_stream struct
  mad_stream_buffer(&mad_stream, input_stream, metadata.st_size);
  FILE* pFile;
    pFile = fopen("file.opus", "wb");
    // Decode frame and synthesize loop
    int s = kNumChannels * kFrameSize;

  while (1) {

    // Decode frame from the stream
    if (mad_frame_decode(&mad_frame, &mad_stream)) {
      if (MAD_RECOVERABLE(mad_stream.error)) {
        continue;
      } else if (mad_stream.error == MAD_ERROR_BUFLEN) {
       break;
      } else {
        break;
      }
    }
    // Synthesize PCM data of frame
    mad_synth_frame(&mad_synth, &mad_frame);
    auto pcm = mad_synth.pcm;
    if (pcm.channels != 2) {
      std::cout << "not two channels, returning\n";
      return;
    }

    int sample_len = mad_synth.pcm.length;
    std::cout << "Mad_snyth sample length: " << sample_len << "\n";
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
//      fwrite(&l, sizeof(opus_int16), 1, pFile);
//      fwrite(&r, sizeof(opus_int16), 1, pFile);
    }
    std::cout << "send\n============";
  }
  std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(audio_set, kFrameSize);
    for(auto entry : opus_out) {
      int len = 0;
      std::cout << "Opus length: " << entry.size() << "\n";
      unsigned char* raw = new unsigned char[entry.size()];
      for(unsigned char c : entry) {
        raw[len] = c;
        len++;
      }
      uint8_t * encodedAudioDataPointer = raw;
//      fwrite (raw, sizeof(unsigned char), sizeof(raw), pFile);
      this->preparePacket(encodedAudioDataPointer, entry.size());
      std::this_thread::sleep_for(std::chrono::milliseconds(17));
    }

  // auto iter = audio_set.begin();
  // for(int i = 0; i < audio_set.size() / 2; i++) {
  //   std::cout << "in loop\n";
  //   std::vector<opus_int16> part (kFrameSize * 2);
  //   for(int x = 0; x < kFrameSize*2; x++) {
  //     part.push_back(*iter);
  //     iter++;
  //   }

  //   std::cout << "total opus packets: " << opus_out.size() << "\n";

  // }

  fclose(pFile);

  fclose(fp);

  // Free MAD structs
  mad_synth_finish(&mad_synth);
  mad_frame_finish(&mad_frame);
  mad_stream_finish(&mad_stream);
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
  sodium_init();
  return true;
}
