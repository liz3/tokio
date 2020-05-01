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
std::stringstream VoiceConnection::preparePacket(unsigned char raw[], int len) {
 //  VoiceConnection::setIntBigEndian(nonceBuffer, 0, encode_count);
  encode_seq++;
  encode_count++;
  timestamp += kFrameSize;
  std::stringstream nonceStream;
  nonceStream << ((unsigned char)0x80) << ((unsigned char)0x78);
  nonceStream.write((const char *)&encode_seq, sizeof(encode_seq));
  nonceStream.write((const char *) &timestamp, sizeof(timestamp));
  nonceStream.write((const char *)&ssrc, sizeof(ssrc));
//  std::cout << std::hex << nonceStream << "\n";
  for(int i = 0; i < 12; i++) {
    nonceStream << (unsigned char) 0x00;
  }
  auto nonce_vec = VoiceConnection::to_vector(nonceStream);
  std::cout << nonce_vec.size() << "\n";
  unsigned char* nonce_arr = &nonce_vec[0];
  // for(int i = 0; i < 24; i++) {
  //   std::cout << std::hex << nonce_arr[i] << " ";
  // }

  std::cout << "\n";
  unsigned char encrypted_data[len];
// int encrypted_len = crypto_stream_xsalsa20_xor(encrypted_data, raw, (unsigned long long)len,  nonce_arr, const_cast<const unsigned char*>((unsigned char*)key.begin()));
  int encrypted_len = crypto_stream_xsalsa20_xor(encrypted_data, raw, (unsigned long long)len,  nonce_arr, &key[0]);
  std::cout << "in func " << encrypted_len << "\n";
  char to_print[30];
  for(int i = 0; i < 30; i++) {
    to_print[i] = (char)encrypted_data[i];
  }
  std::cout << VoiceConnection::string_to_hex(std::string(to_print, 30));
  std::cout << "\n";
  for(int i = 0; i < 30; i++) {
    to_print[i] = (char)raw[i];
  }
  std::cout << VoiceConnection::string_to_hex(std::string(to_print, 30));
  std::cout << "\n";

  nonceStream.write((const char *)encrypted_data, sizeof(const char) * len);
//  free(encrypted_data);
  return nonceStream;
}
void VoiceConnection::play_test() {
//  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  mad_stream_init(&mad_stream);
  mad_synth_init(&mad_synth);
  mad_frame_init(&mad_frame);
  std::vector<opus_int16> audio_set(kFrameSize * kNumChannels);
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
        continue;
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

      opus_int16 out =r<<8|l;
      if(s < 0) {
        next_frame.push_back(out);
        continue;
      }
      s--;
      audio_set.push_back(out);
   //   fwrite(&l, sizeof(opus_int16), 1, pFile);
   //   fwrite(&r, sizeof(opus_int16), 1, pFile);
    }

    if(s < 0) {
      std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(audio_set, kFrameSize);
      std::cout << "total opus packets: " << opus_out.size() << "\n";
      for(auto entry : opus_out) {
        int len = 0;

        std::cout << "Opus length: " << entry.size() << "\n";
        unsigned char raw[entry.size()];
        for(unsigned char c : entry) {
         raw[len] = c;
          len++;
        }

        //   fwrite (raw , sizeof(unsigned char), sizeof(raw), pFile);
       std::stringstream encrypted = this->preparePacket(raw, entry.size());
       auto vec = VoiceConnection::to_vector(encrypted);
       int plen = sizeof(unsigned char) * vec.size();
       std::cout << plen << "\n";
       unsigned char* datap = &vec[0];
        //std::cout << "encrypted length: " << encrypted.length() << "\n";
       this->send(datap, plen);
        //encoder.ResetState();
      }

      audio_set.clear();
      s = kNumChannels * kFrameSize;
      std::cout << "pushing for next entrry: " << next_frame.size() << "\n";
      for (auto entry : next_frame) {
        audio_set.push_back(entry);
      s--;
      }
      next_frame.clear();
    }
    std::cout << "send\n============";
  }
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
