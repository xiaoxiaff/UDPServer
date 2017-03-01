
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include "protocol.h"
/* diep(), #includes and #defines like in the server */


int main(int argc, char *argv[])
{
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;
  Header header;
  char message[1024] = {0};
  char incoming_message[1024] = {0};

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

  header.segmentNum = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
  header.ackNum = 0;
  header.window = 0;
  header.isAck = true;
  header.isRst = false;
  header.isSyn = false;
  header.isFin = false;

  generateHeader(message, &header);
  printHeader(message);
  char *requestFile = "test.txt";
  memcpy(message+HEADER_SIZE, requestFile, strlen(requestFile));
  unsigned int resultAck;
  memcpy(&resultAck, message+4, sizeof(unsigned int));
  printf("\nresultAck:%u", resultAck);
  memcpy(&resultAck, message, sizeof(unsigned int));
  printf("\nSnyc:%u", resultAck);

  // set Connection close state
  /*char *connection = "Connection: close\r\n";
  memcpy(buffer+offset, connection, strlen(connection));
  offset += strlen(connection);

  // set Current date
  time_t now = time(0);
  struct tm *mytime = localtime(&now);
  char outstr[100];
  strftime(outstr, sizeof(outstr), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", mytime);
  memcpy(buffer+offset, outstr, strlen(outstr));
  offset += strlen(outstr);

  // set Server info
  char *server = "Server: yukaiAndrea/1.0\r\n";
  memcpy(buffer+offset, server, strlen(server));
  offset += strlen(server);

  // set Content-Length
  char contentLength[50] = "Content-Length: ";  
  char len[10];
  sprintf (len, "%d", (unsigned int)filesize);
  strcat(contentLength, len);
  strcat(contentLength, "\r\n");

  memcpy(buffer+offset, contentLength, strlen(contentLength));
  offset += strlen(contentLength);

  // set Content-Type based on uri.
  char *contentType;
  if (f == html)  contentType =  "Content-Type: text/html\r\n";
  if (f == txt)  contentType =  "Content-Type: text/plain\r\n";
  if (f == jpeg)  contentType =  "Content-Type: image/jpeg\r\n";
  if (f == jpg)  contentType =  "Content-Type: image/jpg\r\n";
  if (f == gif)  contentType =  "Content-Type: image/gif\r\n";
  memcpy(buffer+offset, contentType, strlen(contentType));
  offset += strlen(contentType);

  // finish the header
  memcpy(buffer+offset, "\r\n\0", 3);*/
  //sends message
  printHeader(message);
  printf("strlen:%d",strlen(message));
  if (send(send_socket, message, HEADER_SIZE + strlen(requestFile), 0) <0) {
      perror("Send failed");
      return 1;
  }
  puts("Message Sent");

  server.sin_port = htons( 8081);

  //checks connection
  if (bind(receive_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  printf("Main\n");
  puts("Bind");

  //receives message back
  if(recv(receive_socket, incoming_message, sizeof(incoming_message), 0) <0) {
    puts("Received failed");
    return 1;
  }
  puts("Message received");
  puts(incoming_message);

  close(send_socket);
  close(receive_socket);
}