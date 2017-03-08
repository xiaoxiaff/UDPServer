#include "protocol.h"

int parseRequest(char incoming_message[], Header* header)
{
  char control[2];

  int offset = 0;

  memcpy(&(header->segmentNum), incoming_message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  memcpy(&(header->ackNum), incoming_message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  memcpy(control, incoming_message+offset, sizeof(control));
  offset += sizeof(control);
  memcpy(&(header->window), incoming_message+offset, sizeof(unsigned short));
  offset += sizeof(short);

  count length = 0;
  length |= (unsigned char)control[1];
  //printf("\n mid: data length:%u, %u, %u\n", length, (unsigned char)control[1], (length<<2));
  header->dataLength = ((length<<2)) | ((unsigned char)control[0]>>6);
  //printf("\n parse: data length:%u %u\n", control[1], header->dataLength);
  //printf("\n char:%u %u\n", (unsigned char)control[1], (unsigned char)control[0]);
  //printf("controlbit:%u\n", control[0]);
  header->isAck = control[0] & 0x10 ? true : false;
  header->isRst = control[0] & 0x4 ? true : false;
  header->isSyn = control[0] & 0x2 ? true : false;
  header->isFin = control[0] & 0x1 ? true : false;

  return 0;
}

void printHeader(char header[])
{
  int i;
  printf("\nheader:");

  for(i=0;i<12;i++)
  {
    printf("%u", header[i]);
  }
  printf("\n");
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

int generatePacket(char message[], Header* header,
				   const char* content, const unsigned int length)
{
  header->dataLength = length;
  //printf("data length:%u", header->dataLength);
  generateHeader(message, header);

  if (content)
  	memcpy(message+HEADER_SIZE, content, length);
  return 0;
}

int generateHeader(char message[], const Header* header)
{
  //printf("generate request\n");

  // set HTML version and statu code
  int offset = 0;
  srand(time(NULL));
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
  controlBit[0] = 0x00;  // reserved = 0
  if(header->isAck) controlBit[0] |= 0x10;
  if(header->isRst) controlBit[0] |= 0x04;
  if(header->isSyn) controlBit[0] |= 0x02;
  if(header->isFin) controlBit[0] |= 0x01;
  memcpy(message+offset, controlBit, sizeof(controlBit));
  offset += sizeof(controlBit);

  char windowSize[2] = {0};

  windowSize[1] = (header->window >> 8) & 0xFF;
  windowSize[0] = header->window & 0xFF;

  memcpy(message+offset, windowSize, sizeof(windowSize));
  offset += sizeof(windowSize);

  return 0;
}

int setHeader(Header* header, count seqNo, count ackNo, count length,
			  bool ack, bool rst, bool syn, bool fin, unsigned short window)
{
  header->segmentNum = seqNo;
  header->ackNum = ackNo;
  header->dataLength = length;
  header->isAck = ack;
  header->isRst = rst;
  header->isSyn = syn;
  header->isFin = fin;
  header->window = window;
}

long gettime() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	long ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
}