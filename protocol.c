#include "protocol.h"
#define TIMEOUT_INTERVAL 500

Header::Header()
{
}

Header::Header(count initID, count seqNo, count ackNo, count window,
	     	   bool ext, bool ack, bool rst, bool syn, bool fin, const count length)
  : initID(initID)
  , segmentNum(seqNo)
  , ackNum(ackNo)
  , window(window)
  , dataLength(length)
  , isExt(ext)
  , isAck(ack)
  , isRst(rst)
  , isSyn(syn)
  , isFin(fin)
{
}

Packet::Packet(char content[])
{
  header = new Header();
  memcpy(message, content, PACKET_LENGTH);
  parseRequest();
}

Packet::Packet(count initID, count seqNo, count ackNo, count window,
	     	   bool ext, bool ack, bool rst, bool syn, bool fin,
	     	   const char* content, const count length)
{
  header = new Header(initID, seqNo, ackNo, window,
  	                  ext, ack, rst, syn, fin, length);
  if (content)
  memcpy(message+HEADER_SIZE, content, length);
}

int
Packet::setMessage(const char* content, const unsigned int length)
{
  header->dataLength = length;

  if (content)
  	memcpy(message+HEADER_SIZE, content, length);
  return 0;
}

int
Packet::encode()
{
  //printf("generate request\n");

  // set HTML version and statu code
  int offset = 0;
  srand(time(NULL));

  char init[4] = {0};
  init[3] = (header->initID >> 24) & 0xFF;
  init[2] = (header->initID >> 16) & 0xFF;
  init[1] = (header->initID >> 8) & 0xFF;
  init[0] = header->initID & 0xFF;

  memcpy(message+offset, init, sizeof(init));
  offset += sizeof(init);
  //unsigned int segmentNum = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
  char seq[4] = {0};
  seq[3] = (header->segmentNum >> 24) & 0xFF;
  seq[2] = (header->segmentNum >> 16) & 0xFF;
  seq[1] = (header->segmentNum >> 8) & 0xFF;
  seq[0] = header->segmentNum & 0xFF;

  //printf("%u, %u %u %u %u, %u", header->segmentNum, seq[0], seq[1], seq[2], seq[3], *((unsigned int*)header->segmentNum));

  memcpy(message+offset, seq, sizeof(seq));
  offset += sizeof(seq);

  char ack[4] = {0};
  ack[3] = (header->ackNum >> 24) & 0xFF;
  ack[2] = (header->ackNum >> 16) & 0xFF;
  ack[1] = (header->ackNum >> 8) & 0xFF;
  ack[0] = header->ackNum & 0xFF;
  memcpy(message+offset, ack, sizeof(ack));
  offset += sizeof(ack);

  char controlBit[2] = {0};
  controlBit[1] = (header->dataLength>>2) & 0xFF;
  controlBit[0] = ((header->dataLength<<6) & 0xC0);
  count length = 0;
  length |= (unsigned char)controlBit[1];
  //printf("\n mid: data length:%u, %u, %u\n", length, (unsigned char)controlBit[1], (length<<2));
  length = ((length<<2)) | ((unsigned char)controlBit[0]>>6);
  //printf("\n generate: data length:%u %u\n", length, header->dataLength);
  //printf("\n char:%u %u\n", (unsigned char)controlBit[1], (unsigned char)controlBit[0]);
  if(header->isExt) controlBit[0] |= 0x20;
  if(header->isAck) controlBit[0] |= 0x10;
  if(header->isRst) controlBit[0] |= 0x04;
  if(header->isSyn) controlBit[0] |= 0x02;
  if(header->isFin) controlBit[0] |= 0x01;
  //printf("\n char:%u %u\n", (unsigned char)controlBit[1], (unsigned char)controlBit[0]);
  memcpy(message+offset, controlBit, sizeof(controlBit));
  offset += sizeof(controlBit);

  char windowSize[2] = {0};

  windowSize[1] = (header->window >> 8) & 0xFF;
  windowSize[0] = header->window & 0xFF;

  memcpy(message+offset, windowSize, sizeof(windowSize));
  offset += sizeof(windowSize);

  return 0;
}

int
Packet::parseRequest()
{
  char control[2];

  int offset = 0;

  memcpy(&(header->initID), message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  memcpy(&(header->segmentNum), message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  memcpy(&(header->ackNum), message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  memcpy(control, message+offset, sizeof(control));
  offset += sizeof(control);
  memcpy(&(header->window), message+offset, sizeof(unsigned short));
  offset += sizeof(short);

  count length = 0;
  length |= (unsigned char)control[1];
  //printf("\n mid: data length:%u, %u, %u\n", length, (unsigned char)control[1], (length<<2));
  header->dataLength = ((length<<2)) | ((unsigned char)control[0]>>6);
  //printf("\n parse: data length:%u %u\n", control[1], header->dataLength);
  //printf("\n char:%u %u\n", (unsigned char)control[1], (unsigned char)control[0]);
  //printf("controlbit:%u\n", control[0]);
  header->isExt = control[0] & 0x20 ? true : false;
  header->isAck = control[0] & 0x10 ? true : false;
  header->isRst = control[0] & 0x4 ? true : false;
  header->isSyn = control[0] & 0x2 ? true : false;
  header->isFin = control[0] & 0x1 ? true : false;

  return 0;
}

void
Packet::printHeader()
{
  int i;
  std::cout<<"header size"<<HEADER_SIZE<<std::endl;

  for(i=0;i<HEADER_SIZE;i++)
  {
    printf("%d:%u,", i, message[i]);
    if (!((i+1)%4))
      printf("\n");
  }
  printf("\n");
}

void
Packet::print()
{
  std::cout<<"========================="<<std::endl;
  //std::cout<<"id:"<<header->initID<<std::endl;
  std::cout<<"seq:"<<header->segmentNum<<std::endl;
  std::cout<<"ack:"<<header->ackNum<<std::endl;
  //std::cout<<"dataLength:"<<header->dataLength<<std::endl;
  //std::cout<<"window:"<<header->window<<std::endl;
  //std::cout<<"syn"<<header->isSyn<<std::endl;
  //std::cout<<"ack:"<<header->isAck<<std::endl;
  //std::cout<<"fin:"<<header->isFin<<std::endl;
}

Node::Node(Packet* packet, bool needAck)
{
  this->packet = packet;
  this->needAck = needAck;
  this->retransmittime = 0;
}

Node::~Node()
{
  delete this->packet;
}

void printMessage(char message[], size_t length)
{
  int i;
  printf("\nmessage:");

  for(i=0;i<length;i++)
  {
    printf("%u", message[i]);
  }
  printf("\n");
}

int get_time()
{
  timeval curTime;
  gettimeofday(&curTime, NULL);
  return curTime.tv_usec / 1000;
}

void sendPacket(sendArgs* args, Packet* packet, bool needAck)
{
  Node* node = new Node(packet, needAck);
  //std::cout<<packet<<" "<<node<<std::endl;
  //puts("send packet");
  args->lock.lock();
    args->send_queue.push(node);
    if (needAck) {
      //std::cout<<"seq:"<<packet->header->segmentNum<<", data:"<<packet->header->dataLength<<" wait for ack"<<std::endl;
      node->timestamp = gettime() + TIMEOUT_INTERVAL;
      args->time_queue.push_back(node);
    }
  args->lock.unlock();
  //std::cout<<"send queue length:"<<args->send_queue.size()<<std::endl;
}

void send_data(sendArgs* args) {
  puts("send_data");
  while(args->isRunning) {
      Node* node = NULL;
      args->lock.lock();
      if (!args->send_queue.empty()) {
        //std::cout<<"get node from send queue"<<std::endl;
        node = args->send_queue.front();
        args->send_queue.pop();
      }
      args->lock.unlock();
      if (node)
      {
      	Header* h = node->packet->header;
      	if(args->isServer)
      	  std::cout<<"Sending packet "<<h->segmentNum<<" "<<h->window;
      	else
      	  std::cout<<"Sending packet "<<h->ackNum<<" "<<h->window;
        if(node->retransmittime != 0)
          std::cout<<" Retransmission";
      	if(h->isSyn)
      	  std::cout<<" SYN";
      	if(h->isFin)
      	  std::cout<<" FIN";
      	std::cout<<std::endl;

        node->packet->encode();

        //node->packet->print();
        //std::cout<<node->packet->header->dataLength<<std::endl;
        if(send(args->socket, (node->packet)->message,
        	    HEADER_SIZE + (node->packet->header)->dataLength, 0) <0) {
          perror("Send failed\n");
        }
  		  //std::cout<<node<<"after send data"<<args->send_queue.size()<<args->send_queue.empty()<<std::endl;
        //printLog(true, node->packet->header);
        if (!node->needAck)
          delete node;
     }
  }
  std::cout<<"exit send_data"<<std::endl;
}

void receive_data(recvArgs* args) {
  char incoming_message[PACKET_LENGTH];

  puts("receive_data");
  struct sockaddr_in from;            /* Sender's address. */
  socklen_t fromlen;              /* Length of sender's address. */
  fromlen = sizeof(from);
  while (args->isRunning) {
    if ( recvfrom(args->socket, incoming_message, sizeof(incoming_message), 0, (struct sockaddr *)&from, &fromlen) < 0) {
      puts("Received failed");
      exit(1);
    }
    //std::cout<<"get data"<<std::endl;
    //printLog(false, &header);
    //std::cout<<std::string(inet_ntoa(from.sin_addr))<<", "<<from.sin_port<<std::endl;
    Packet* packet = new Packet(incoming_message);
    Node *node = new Node(packet, false);

    //node->packet->print();
    Header* h = node->packet->header;
    if(args->isServer)
      std::cout<<"Receiving packet "<<h->ackNum;
    else
      std::cout<<"Receiving packet "<<h->segmentNum;
    std::cout<<std::endl;

    args->lock.lock();
      if (args->clientIp != std::string(inet_ntoa(from.sin_addr)))
        args->clientIp = std::string(inet_ntoa(from.sin_addr));
      args->queue.push(node);
    args->lock.unlock();
  }
  std::cout<<"exit receive_data"<<std::endl;

}

void check_timeout(sendArgs* args) {
  puts("check_timeout");
  while (args->isRunning) {
    args->lock.lock();
      if (!args->time_queue.empty()) {
        auto it = args->time_queue.begin();
        //std::cout<<(*it)->timestamp <<", "<< gettime()<<std::endl;
        if((*it)->timestamp < gettime()) {
          //std::cout<<"time_out!!!!!!!!!!!!!!!!!!!!\nseq:"<<(*it)->packet->header->segmentNum<<std::endl;
          Node* node = (*it);
          node->retransmittime += 1;
          if (node->retransmittime > RETRANSMITTIME) {
            //std::cout<<"retransmit time out"<<std::endl;
            args->time_queue.erase(it);
            if (!args->isServer)
              args->isRunning = false;
          }
          else if (node->retransmittime > FINRETRANSMITTIME && node->packet->header->isFin && !args->isServer) {
            std::cout<<"Fin time out at client"<<std::endl;
            args->time_queue.erase(it);
            args->isRunning = false;
          }
          else {
            args->send_queue.push(node);
            args->time_queue.erase(it);
            node->timestamp = gettime() + TIMEOUT_INTERVAL;
            args->time_queue.push_back(node);
          }
        }
      }
    args->lock.unlock();
  }
  std::cout<<"exit checkout time"<<std::endl;
}

bool checkBuffer(sendArgs* args, Node* node)
{
  Header* header = node->packet->header;
  //if (header->isAck)
  //  return false;
  args->lock.lock();
    //std::cout<<"time q size"<<args->time_queue.size()<<std::endl;
    auto it1 = args->time_queue.begin();
    while (it1 != args->time_queue.end() &&
         ((*it1)->packet->header->segmentNum + (*it1)->packet->header->dataLength)%MAX_SEGMENT_NUMBER != header->ackNum) {
      it1++;
    }
    if (it1 != args->time_queue.end()) {
      //std::cout<<"erase!!!!\nack:"<<header->ackNum<<" eliminate seq:"<<(*it1)->packet->header->segmentNum<<std::endl;
      delete *it1;
      args->time_queue.erase(it1);
      //std::cout<<"after erase"<<std::endl;
    }
    //std::cout<<"after process time q size"<<args->time_queue.size()<<std::endl;
  args->lock.unlock();
  return true;

}

long long gettime() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

bool lessthan(count c1, count c2) {
  if (c1 == c2) return false;
  if (c2 >= (MAX_SEGMENT_NUMBER/2))
    return c1<c2 && c1 >= (c2-(MAX_SEGMENT_NUMBER/2));
  if (c2 < (MAX_SEGMENT_NUMBER/2))
    return !(c1>c2 && c1 <= (c2+(MAX_SEGMENT_NUMBER/2)));
}

bool greaterthan(count c1, count c2) {
  if (c1 == c2) return false;
  if (c2 >= (MAX_SEGMENT_NUMBER/2))
    return !(c1<c2 && c1 >= (c2-(MAX_SEGMENT_NUMBER/2)));
  if (c2 < (MAX_SEGMENT_NUMBER/2))
    return c1>c2 && c1 <= (c2+(MAX_SEGMENT_NUMBER/2));
}