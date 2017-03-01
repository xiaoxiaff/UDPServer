/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include "protocol.h"

int main(int argc, char *argv[])
{
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;
  Header header;
  char incoming_message[1024];

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
      //Receive an incoming message
  if( recv(receive_socket, incoming_message, sizeof(incoming_message), 0) < 0) {      
    puts("Received failed");
    return 1;
  }
  printf("Received\n");
  printHeader(incoming_message);
  unsigned int receiveAck;
  memcpy(&receiveAck, incoming_message+4, sizeof(unsigned int));
  printf("\nresultAck:%u", receiveAck);
  memcpy(&receiveAck, incoming_message, sizeof(unsigned int));
  printf("\nSnyc:%u", receiveAck);

  parseRequest(incoming_message, &header);

  printf("%u,%u, %d, %d, %d, %d ,%hu", 
    header.segmentNum, header.ackNum, header.isAck,
    header.isSyn, header.isRst, header.isFin, header.window);
  //puts(incoming_message);

  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_port = htons(8081);

  if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  puts("Connected");

  //Sends message back
  char message[100];

  printf("Input Message: ");
  fgets(message, 100, stdin);

  if(send(send_socket, message, strlen(message), 0) <0) {        
    perror("Send failed");
    return 1;
  }
  puts("Message Sent");

  close(send_socket);
  close(receive_socket);
}