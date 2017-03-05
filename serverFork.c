/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include "protocol.h"
#include <unistd.h>

int fileRead(FILE* pf, long int offset, char* message)
{
	   /* Seek to the beginning of the file */
   fseek(pf, offset, SEEK_SET);
   /* Read and display data */
   return fread(message+HEADER_SIZE, 1, PACKET_LENGTH-HEADER_SIZE, pf);

}

int printLog(bool isSend, Header* header)
{
  if (isSend) {
  	printf("Sending packet %u %u ", header->segmentNum, header->window);
  	if (header->isSyn) printf("SYN");
  	if (header->isFin) printf("FIN");
  	printf("\n");
  }
  else {
  	printf("Receiving packet %u\n", header->ackNum);
  }
}

int main(int argc, char *argv[])
{
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;
  Header header;
  char incoming_message[PACKET_LENGTH];

  //create socket
  send_socket = socket(AF_INET, SOCK_DGRAM, 0);
  receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (send_socket == -1 || receive_socket == -1) {
    printf("Could not create socket");
    return 1;
  }

  //assign values
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8080);

  //checks connection
  if (bind(receive_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  printf("Main\n");
  puts("Bind");

  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(8081);

  if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  puts("Connected");

  while (true)
  {
  	count recNo = 0, seqNo = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
  	bool connected = false;
	if( recv(receive_socket, incoming_message, sizeof(incoming_message), 0) < 0) {      
      puts("Received failed");
      return 1;
    }

    FILE *pf;
    parseRequest(incoming_message, &header);
    printLog(false, &header);
    if (header.isSyn) {

      char filename[100];
      strncpy(filename, incoming_message+HEADER_SIZE, header.dataLength);
      filename[header.dataLength] = '\0';

      pf=fopen(filename, "r");
      if (pf==NULL) {
      	fputs ("File error",stderr);
      	continue;
      }
      recNo = header.segmentNum;
      connected = true;
    }

    if(!connected) {
      continue;
   	}

   	char message[PACKET_LENGTH];

  	setHeader(&header, seqNo, ++recNo, 0, true, false, true, false, 0);
  	generatePacket(message, &header, NULL, 0);

    fseek (pf , 0 , SEEK_END);
    long fileSize = ftell (pf);
    long cur = 0;
    rewind (pf);


    printLog(true, &header);
    if(send(send_socket, message, HEADER_SIZE, 0) <0) {        
      perror("Send failed");
      return 1;
  	}

  	while (cur < fileSize) {
  	  if ( recv(receive_socket, incoming_message, sizeof(incoming_message), 0) < 0) {      
        puts("Received failed");
        return 1;
      }

      parseRequest(incoming_message, &header);
      printLog(false, &header);

      //printf("received seq:%u, cur:%ld, total:%ld", header.segmentNum, cur, fileSize);
      if (header.ackNum == seqNo + 1) {
        size_t length = fileRead(pf, cur, message);
        //printLog(message, HEADER_SIZE+length);
        cur += length;
	  	setHeader(&header, ++seqNo, ++recNo, 0, true, false, false, false, 0);
	  	generatePacket(message, &header, message+HEADER_SIZE, length);
      } else {
      	continue;
      }
      if(send(send_socket, message, HEADER_SIZE + header.dataLength, 0) <0) {        
        perror("Send failed");
        return 1;
      }

      printLog(true, &header);

  	}

  	fclose (pf);
  }
  


  //Sends message back
  /*char message[PACKET_LENGTH];
  if (message == NULL) {fputs ("Memory error",stderr); exit (2);}

  size_t result = fread (message,1,PACKET_LENGTH,fp);
  printf("read result:%d\n", result);

  if(send(send_socket, message, result, 0) <0) {        
    perror("Send failed");
    return 1;
  }
  puts("Message Sent"); */

  close(send_socket);
  close(receive_socket);
}