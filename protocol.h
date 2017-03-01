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

typedef struct ProtocolHeader {
   unsigned int segmentNum;
   unsigned int ackNum;
   bool isAck;
   bool isRst;
   bool isSyn;
   bool isFin;
   unsigned short window;
} Header;

int parseRequest(char incoming_message[], Header* header);

void printHeader(char header[]);

int generateHeader(char message[], const Header* header);

#endif