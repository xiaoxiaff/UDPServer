
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include "protocol.h"
#include <unistd.h> 
#include <climits>
#include <chrono>
/* diep(), #includes and #defines like in the server */
#define WINDOWS 5120

typedef struct BufferNode {
  count segNum;
  count internal;
  char message[PACKET_LENGTH];
} BufferNode;


int appendToFileEnd(FILE *pf, const char incoming_message[], unsigned int length)
{
	fseek(pf, 0, SEEK_END);
	fwrite(incoming_message+HEADER_SIZE, 1 , length , pf );
}

int requestFile(sendArgs* send_args, recvArgs* recv_args, std::string requestFile, FILE* pf)
{
  char message[PACKET_LENGTH] = {0};
  char incoming_message[PACKET_LENGTH] = {0};

  count seqNo = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
  count initID = seqNo;
  count endAckNum = UINT_MAX;
  count rcv_base = 0;

  Packet* packet = new Packet(initID, seqNo, 0, WINDOWS, 
  	                          false, false, false, true, false);

  std::cout<<packet->header->dataLength<<std::endl;
  //puts("before send Packet");
  sendPacket(send_args, packet, true);

  std::vector<BufferNode*> buffer;

  while (send_args->isRunning) {
  	Node* node = NULL;
    recv_args->lock.lock();
	    if (!recv_args->queue.empty()) {
        node = recv_args->queue.front();
	      recv_args->queue.pop();
	    }
    recv_args->lock.unlock();

  	if (!node) {
	    continue;
    }
    //std::cout<<"get node and process"<<std::endl;
    //node->packet->print();

    Header* header = node->packet->header;

    checkBuffer(send_args, node);

    if (header->isSyn && header->isAck) {
        Packet* packet = new Packet(header->initID, header->ackNum,
                                    (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                    WINDOWS, false, true, false, false, false,
     								requestFile.c_str(), strlen(requestFile.c_str()));
        sendPacket(send_args, packet, false);
        rcv_base = (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER;
    }

    else if (header->isFin && header->isAck) {
      delete node;
      break;
    }

    else if (header->isFin) {

      Packet* packet1 = new Packet(header->initID, header->ackNum,
                                    (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                    header->window, false, true, false, false, true);
      sendPacket(send_args, packet1, false);

      Packet* packet2 = new Packet(header->initID, (header->ackNum + 1) % MAX_SEGMENT_NUMBER,
                          (header->segmentNum + header->dataLength + 1) % MAX_SEGMENT_NUMBER,
                          header->window, false, false, false, false, true);
      sendPacket(send_args, packet2, true);
      //std::cout<<"send back fin from client"<<std::endl;
    }
    else {
      
        Packet* packet = new Packet(header->initID, header->ackNum,
                                    (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                    WINDOWS, false, true, false, false, false);
        sendPacket(send_args, packet, false);

      if (header->segmentNum == rcv_base || greaterthan(header->segmentNum, rcv_base)) {
        auto it = buffer.begin();
        while (it != buffer.end() && lessthan((*it)->segNum, header->segmentNum)) {
          it++;
        }
        if(it == buffer.end() || (*it)->segNum != header->segmentNum) {
          BufferNode* buffernode = new BufferNode();
          buffernode->segNum = header->segmentNum;
          buffernode->internal = header->dataLength;
          memcpy(buffernode->message, node->packet->message, HEADER_SIZE + header->dataLength);
          buffer.insert(it,buffernode);
        }
        it = buffer.begin();
        while (it != buffer.end() && (*it)->segNum == rcv_base) {
          rcv_base = (rcv_base + (*it)->internal) % MAX_SEGMENT_NUMBER;
          //std::cout<<"rec based update to:"<<rcv_base<<std::endl;
          appendToFileEnd(pf, (*it)->message, (*it)->internal);
          delete *it;
          buffer.erase(it);
          it = buffer.begin();
        }
      }
        //buffer to file;
    }
    delete node;
  }
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
  server.sin_addr.s_addr = htonl(INADDR_ANY);
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

  sendArgs send_args;
  recvArgs recv_args;
  send_args.socket = send_socket;
  send_args.isRunning = true;
  send_args.isServer = false;
  recv_args.socket = receive_socket;
  recv_args.isRunning = true;
  recv_args.isServer = false;

  std::thread t1(send_data, &send_args);
  std::thread t2(receive_data, &recv_args);
  std::thread t3(check_timeout, &send_args);
  requestFile(&send_args, &recv_args, "test.txt", pf);

  fclose (pf);
  puts("finished");

  recv_args.isRunning = false;
  send_args.isRunning = false;
  shutdown(receive_socket, SHUT_RDWR);
  close(send_socket);
  close(receive_socket);
  t1.join();
  t2.join();
  t3.join();
}