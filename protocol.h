#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <stdbool.h>

#define MAX_SEGMENT_NUMBER 30720
#define HEADER_SIZE 12
#define PACKET_LENGTH 1024

typedef unsigned int count;
/**

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   |U|A|P|R|S|F|                               |
|    data length    |R|C|S|S|Y|I|            Window             |
|                   |G|K|H|T|N|N|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct ProtocolHeader {
   count segmentNum;
   count ackNum;
   count dataLength;
   bool isAck;
   bool isRst;
   bool isSyn;
   bool isFin;
   unsigned short window;
} Header;

typedef struct Packet {
	int length;
	count segmentNum;
	long timestamp;
	char message[PACKET_LENGTH];
} Packet;

typedef struct Node {
	Packet* packet;
	struct Node* prev;
	struct Node* next;
} Node;

int parseRequest(char incoming_message[], Header* header);

void printHeader(char header[]);

void printMessage(char message[], size_t length);

int generatePacket(char message[], Header* header,
				   const char* content, const count length);

int generateHeader(char message[], const Header* header);

int setHeader(Header* header, count seqNo, count ackNo, count length,
			  bool ack, bool rst, bool syn, bool fin, unsigned short window);
			  
long gettime();

#endif