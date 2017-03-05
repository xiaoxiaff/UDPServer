
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include "protocol.h"
/* diep(), #includes and #defines like in the server */

int appendToFileEnd(FILE *pf, const char incoming_message[], unsigned int length)
{
	fseek(pf, 0, SEEK_END);
	fwrite(incoming_message+HEADER_SIZE, 1 , length , pf );
}

int requestFile(int send_socket, int receive_socket, char* requestFile, FILE* pf)
{
  Header header;
  char message[PACKET_LENGTH] = {0};
  char incoming_message[PACKET_LENGTH] = {0};

  count seqNo = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
  count recSeqNo;

  setHeader(&header, seqNo, 0, 0, false, false, true, false, 0);
  generatePacket(message, &header, requestFile, strlen(requestFile));

  if (send(send_socket, message, HEADER_SIZE + strlen(requestFile), 0) <0) {
      perror("Send failed");
      return 1;
  }
  puts("Message Sent");

  //receives message back
  if (recv(receive_socket, incoming_message, sizeof(incoming_message), 0) <0) {
    puts("Received failed");
    return 1;
  }
  puts("Message received");
  parseRequest(incoming_message, &header);
  if (header.isAck && header.isSyn && header.ackNum == seqNo+1) {
  	recSeqNo = header.segmentNum;
  }
  else {
  	return 1;
  }

  do {
  	setHeader(&header, ++seqNo, ++recSeqNo, 0, false, false, false, false, 0);
  	generatePacket(message, &header, NULL, 0);
  	if (send(send_socket, message, HEADER_SIZE + strlen(requestFile), 0) <0) {
      perror("Send failed");
      return 1;
  	}
  	puts("Message Sent");
  	  //receives message back
	if (recv(receive_socket, incoming_message, sizeof(incoming_message), 0) <0) {
	  puts("Received failed");
	  return 1;
	}
	puts("Message received");
	//printHeader(incoming_message);
	parseRequest(incoming_message, &header);
	printf("received seq:%u, data length:%u, max: %u", header.segmentNum, header.dataLength, PACKET_LENGTH - HEADER_SIZE);
	appendToFileEnd(pf, incoming_message, header.dataLength);
  } while (header.dataLength == PACKET_LENGTH - HEADER_SIZE);
  return 0;
}

int main(int argc, char *argv[])
{
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;

  FILE * pf;
  pf = fopen ("received.txt", "w");


  //create socket
  send_socket = socket(AF_INET, SOCK_DGRAM, 0);
  receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (send_socket == -1 || receive_socket == -1) {
    printf("Could not create socket");
    return 1;
  }

  //assign local values
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons( 8080 );

  //checks connection
  if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
      perror("Connection error");
      return 1;
  }
  puts("Connected");

  server.sin_port = htons( 8081);

  //checks connection
  if (bind(receive_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  printf("Main\n");
  puts("Bind");

  requestFile(send_socket, receive_socket, "test.txt", pf);

  fclose (pf);
  puts("finished");

  close(send_socket);
  close(receive_socket);
}