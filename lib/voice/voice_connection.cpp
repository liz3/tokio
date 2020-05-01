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

 sendto(sockfd, (const char *)buffer, size,
        MSG_WAITALL, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
}
std::string VoiceConnection::preparePacket(unsigned char raw[], int len) {
  unsigned char nonceBuffer[24] = {0};
  encode_seq++;
  encode_count++;
  timestamp += kFrameSize;
  VoiceConnection::setIntBigEndian(nonceBuffer, 0, encode_count);

  unsigned char* encrypted;
  int encrypt_len = crypto_stream_xsalsa20_xor(encrypted, raw, (unsigned long long) len, nonceBuffer, key);
  char f[12 + len + 4];
  std::stringbuf buffer;
  buffer.pubsetbuf(f, 12 + encrypt_len + 4);
  std::ostream os (&buffer);
  os << ((unsigned char)0x80);
  os << ((unsigned char)0x78);
  os << ((unsigned char)encode_seq);
  os << timestamp;
  os << ssrc;
//  buffer.sputn ((const char *)encrypted, encrypt_len);
  for(int i = 0; i < encrypt_len; i++) {
    os << encrypted[i];
  }
//  std::string str = std::string(encrypted, encrypt_len);
 // os << encrypted;
  os << 0 << 0 << 0 << 0;
  os << nonceBuffer;
  return buffer.str();
}
void VoiceConnection::play_test() {
  mad_stream_init(&mad_stream);
  mad_synth_init(&mad_synth);
  mad_frame_init(&mad_frame);
  std::vector<opus_int16> audio_set(kFrameSize * kNumChannels);
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
    audio_set.clear();

    int sample_len = mad_synth.pcm.length;
    std::cout << "Mad_snyth sample length: " << sample_len << "\n";
    mad_fixed_t const *left_ch = pcm.samples[0], *right_ch = pcm.samples[1];

   int s = kNumChannels * kFrameSize;
    while(sample_len--) {
     if(s < 0) break;
      s--;
      int left;
      int right;
      left = (*left_ch++);
      right = (*right_ch++);
      opus_int16 l,r;
      l = (opus_int16)(left >> 16);
      r = (opus_int16)(right >> 16);

 //     opus_int16 out =l<<8|r;
   //   audio_set.push_back(l);
      fwrite(&l, sizeof(opus_int16), 1, pFile);
      fwrite(&r, sizeof(opus_int16), 1, pFile);
    }

    std::vector<std::vector<unsigned char>> opus_out = encoder.Encode(audio_set, kFrameSize);

    int len = 0;
    for(auto entry : opus_out) {
      len += entry.size();
    }
    std::cout << "Opus length: " << len << "\n";
    unsigned char raw[len];
    len = 0;
    for(auto entry : opus_out) {
      for(unsigned const char c : entry) {
        raw[len] = c;
        len++;
      }
    }
 //   fwrite (raw , sizeof(unsigned char), sizeof(raw), pFile);
 //   std::string encrypted = this->preparePacket(raw, len);
    //std::cout << "encrypted length: " << encrypted.length() << "\n";
//    this->send((unsigned  char *)encrypted.c_str(), encrypted.length() - 1);
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
  return true;
}
