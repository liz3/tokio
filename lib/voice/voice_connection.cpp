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

                    char buff[] = {( char)0xC9, 0, 0, 0, 0, 0, 0, 0, 0};
                    finalThis->send(buff, 9);
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                  }
                });
  t.detach();
}
void VoiceConnection::send(char buffer[], int size) {

 sendto(sockfd, (const char *)buffer, size,
        MSG_WAITALL, ( struct sockaddr *) &servaddr,
         sizeof(servaddr));
}
void VoiceConnection::preparePacket(unsigned char raw[], int len) {
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
//  buffer.sputn (encrypted, encrypt_len);
//  std::string str = std::string(encrypted, encrypt_len);
  os << encrypted;
  os << 0 << 0 << 0 << 0;
  os << nonceBuffer;

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
