#include "protocol.h"

int parseRequest(char incoming_message[], Header* header)
{
  unsigned int sync = 0, ack = 0;
  char control[2];
  unsigned short window = 0;

  int offset = 0;

  memcpy(&(header->segmentNum), incoming_message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  printf("offset:%d", offset);
  memcpy(&(header->ackNum), incoming_message+offset, sizeof(unsigned int));
  offset += sizeof(unsigned int);
  printf("offset:%d", offset);
  memcpy(control, incoming_message+offset, sizeof(control));
  offset += sizeof(control);
  printf("offset:%d", offset);
  memcpy(&(header->window), incoming_message+offset, sizeof(unsigned short));
  offset += sizeof(short);
  printf("offset:%d\n", offset);

  printf("controlbit:%u\n", control[0]);
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


int generateHeader(char message[], const Header* header)
{
  printf("generate request\n");

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
  printf("offset:%d", offset);

  char ack[4] = {0};
  ack[3] = (header->ackNum >> 24) & 0xFF;
  ack[2] = (header->ackNum >> 16) & 0xFF;
  ack[1] = (header->ackNum >> 8) & 0xFF;
  ack[0] = header->ackNum & 0xFF;
  memcpy(message+offset, ack, sizeof(ack));
  offset += sizeof(ack);
  printf("offset:%d", offset);

  char controlBit[2] = {};
  controlBit[1] = 0x30;  // data offset = 3, reserved = 0
  controlBit[0] = 0x00;  // reserved = 0
  if(header->isAck) controlBit[0] |= 0x10;
  if(header->isRst) controlBit[0] |= 0x04;
  if(header->isSyn) controlBit[0] |= 0x02;
  if(header->isFin) controlBit[0] |= 0x01;
  printf("controlbit:%u %u\n", controlBit[1], controlBit[0]);
  memcpy(message+offset, controlBit, sizeof(controlBit));
  offset += sizeof(controlBit);
  printf("offset:%d", offset);

  char windowSize[2] = {0};

  windowSize[1] = (header->window >> 8) & 0xFF;
  windowSize[0] = header->window & 0xFF;

  memcpy(message+offset, windowSize, sizeof(windowSize));
  offset += sizeof(windowSize);

  return 0;
}