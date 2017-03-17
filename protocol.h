#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h> /* for the waitpid() system call */
#include <stdbool.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <queue>
#include <map>
#include <string>
#include <arpa/inet.h>

#define MAX_SEGMENT_NUMBER 30720
#define HEADER_SIZE 16
#define PACKET_LENGTH 1024

#define RETRANSMITTIME 50
#define CLIENTPORT 8081

typedef unsigned int count;
/**

0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Init ID                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Sequence Number                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgment Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   |E|A|P|R|S|F|                               |
|    data length    |X|C|S|S|Y|I|            Window             |
|                   |T|K|H|T|N|N|                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
class Header {
public:
   count initID;
   count segmentNum;
   count ackNum;
   count dataLength;
   bool isExt;
   bool isAck;
   bool isRst;
   bool isSyn;
   bool isFin;
   unsigned short window;
public:
  Header();
  Header(count initID, count seqNo, count ackNo, count window,
	     bool ext = false, bool ack = false, bool rst = false,
	     bool syn = false, bool fin = false, const count length = 0);
};

class Packet {
public:
	Header* header;
	char message[PACKET_LENGTH];

public:
Packet(char message[]);
Packet(count initID, count seqNo, count ackNo, count window,
	   bool ext = false, bool ack = false, bool rst = false,
	   bool syn = false, bool fin = false,
	   const char* content = NULL, const count length = 1);

int
setMessage(const char* content, const count length);

int
encode();

int
parseRequest();

void
printHeader();

void
print();
};

class Node {
public:
long long timestamp;
Packet* packet;
bool needAck;
int retransmittime;
public:
Node(Packet*, bool);
~Node();
};

typedef struct recvArgs {
  bool isServer;
  int socket;
  std::queue<Node*> queue;
  std::mutex lock;
  std::string clientIp;
  bool isRunning;
} recvArgs;

typedef struct sendArgs {
  bool isServer;
  int socket;
  std::queue<Node*> send_queue;
  std::vector<Node*> time_queue;
  std::vector<Node*> receive_buffer;
  count sendBase;
  std::mutex lock;
  bool isRunning;
} sendArgs;

int get_time();

void sendPacket(sendArgs* args, Packet* packet, bool needAck);

void send_data(sendArgs* args);

void receive_data(recvArgs* args);

void check_timeout(sendArgs* args);

void printMessage(char message[], size_t length);		  
//long gettime();

bool checkBuffer(sendArgs* args, Node* node);

long long gettime();

bool lessthan(count c1, count c2);
bool greaterthan(count c1, count c2);

#endif