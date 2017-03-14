/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include "protocol.h"
#include <unistd.h>

std::map<count, std::string> id_filename;
std::map<count, long> id_filesize;
std::map<count, count> id_nextsegment;
std::map<count, count> id_offset;
std::map<count, count> id_sendbase;

typedef struct BufferNode {
  count ackNum;
  bool isAcked;
} BufferNode;

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
  char incoming_message[PACKET_LENGTH];
  pthread_t tid[2];
  std::map<count, std::string>::iterator filename_it;
  std::map<count, long>::iterator filesize_it;
  std::map<count, count>::iterator nextsegment_it;
  std::map<count, count>::iterator sendbase_it;

  std::vector<BufferNode*> buffer;

  count window = 0;

  srand (time(NULL));
  //create socket
  send_socket = socket(AF_INET, SOCK_DGRAM, 0);
  receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (send_socket == -1 || receive_socket == -1) {
    printf("Could not create socket");
    return 1;
  }

  //assign values
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_family = AF_INET;
  server.sin_port = htons(8080);

  //checks connection
  if (bind(receive_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  printf("Main\n");
  puts("Bind");

  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(8081);

  if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  puts("Connected");

  sendArgs send_args;
  recvArgs recv_args;
  send_args.socket = send_socket;
  send_args.isRunning = true;
  send_args.isServer = true;
  recv_args.socket = receive_socket;
  recv_args.isRunning = true;
  recv_args.isServer = true;

  puts("before thread");
  std::thread t1(send_data, &send_args);
  std::thread t2(receive_data, &recv_args);
  std::thread t3(check_timeout, &send_args);
  puts("after thread");

  while (true) {
  	Node* node = NULL;
    recv_args.lock.lock();
	  if (!recv_args.queue.empty()) {
      node = recv_args.queue.front();
		  recv_args.queue.pop();
	  }
    recv_args.lock.unlock();

	  if (!node)
	    continue;

    FILE *pf = NULL;
    Header* header = node->packet->header;
    checkBuffer(&send_args, node);

    std::cout<<"server process node"<<std::endl;
    //node->packet->print();
    if (header->isSyn) {

      std::cout<<"isSyn"<<std::endl;
      sendbase_it = id_sendbase.find(header->initID);
      if (sendbase_it == id_sendbase.end()) { 
        //printf("Syn packet\n");
        //printf("before file ");
        count seq = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
        Packet* packet = new Packet(header->initID, seq, 
                                    header->segmentNum + header->dataLength,
                                    header->window,
                                    false, true, false, true, false);
        sendPacket(&send_args, packet, true);
        id_sendbase[header->initID] = 0;
      }
      //std::cout<<"finished syn"<<std::endl;
    }
    else if (header->isFin) {
      std::cout<<"isFin"<<std::endl;
      if (header->isAck || buffer.size()>0) {
        delete node;
        continue;
      }

      Packet* packet1 = new Packet(header->initID, header->ackNum,
                                    header->segmentNum + header->dataLength,
                                    header->window, false, true, false, false, true);
      sendPacket(&send_args, packet1, false);

      Packet* packet2 = new Packet(header->initID, header->ackNum+1,
                          header->segmentNum + header->dataLength+1,
                          header->window, false, false, false, false, true);
      sendPacket(&send_args, packet2, true);
      std::cout<<"before erase"<<std::endl;
      id_nextsegment.erase (header->initID);
      std::cout<<"erase offset"<<std::endl;
      id_filename.erase (header->initID);
      std::cout<<"erase filenmae"<<std::endl;
      id_sendbase.erase (header->initID);
      std::cout<<"erase window"<<std::endl;
      id_filesize.erase (header->initID);
      std::cout<<"erase fileSize"<<std::endl;
      id_offset.erase (header->initID);
      std::cout<<"erase fileSize"<<std::endl;
    }

    else if (header->isAck) {
      //std::cout<<"isAck"<<std::endl;
      filename_it = id_filename.find(header->initID);
      window = header->window;
      if (filename_it != id_filename.end())
        pf = fopen(filename_it->second.c_str(), "r");
      else {
        char filename[100];
        strncpy(filename, node->packet->message+HEADER_SIZE, header->dataLength);
        filename[header->dataLength] = '\0';
        //printf("length:%d, filename:%s\n", header->dataLength, filename);
        pf=fopen(filename, "r");
        id_filename[header->initID] = std::string(filename);
        fseek (pf , 0 , SEEK_END);
        long fileSize = ftell (pf);
        long cur = 0;
        rewind (pf);
        id_filesize[header->initID] = fileSize;
        id_sendbase[header->initID] = header->ackNum;
        id_nextsegment[header->initID] = header->ackNum;
        id_offset[header->initID] = header->ackNum;
      }

      //std::cout<<"after filename"<<std::endl;
      if (pf==NULL || header->ackNum < id_sendbase[header->initID]) {
        fputs ("error occured",stderr);
        delete node;
        continue;
      }

      auto it = buffer.begin();
      while (it != buffer.end() && (*it)->ackNum != header->ackNum) {
        it++;
      }
      if (it != buffer.end() && (*it)->isAcked == true) {
        fputs ("duplicated ack recevied",stderr);
        delete node;
        continue;
      }
      if (it != buffer.end())
        (*it)->isAcked = true;
      it = buffer.begin();
      while (it != buffer.end() && (*it)->isAcked == true) {
        id_sendbase[header->initID] = (*it)->ackNum;
        std::cout<<"update sendbase to "<<id_sendbase[header->initID]<<std::endl;
        delete *it;
        buffer.erase(it);
        it = buffer.begin();
      }

      while (id_nextsegment[header->initID] - id_sendbase[header->initID] < window)
      {
        Packet* packet = new Packet(header->initID, id_nextsegment[header->initID],
                                    header->segmentNum + header->dataLength,
                                    header->window);

        size_t length = fileRead(pf, id_nextsegment[header->initID] - id_offset[header->initID], packet->message);
        if (id_nextsegment[header->initID] + length - id_sendbase[header->initID]> window || length == 0) {
          delete packet;
          break;
        }
        else{
          packet->header->dataLength = length;
          BufferNode* buffernode = new BufferNode();
          buffernode->ackNum = id_nextsegment[header->initID] + length;
          buffernode->isAcked = false;
          buffer.push_back(buffernode);
          id_nextsegment[header->initID] += length;
          sendPacket(&send_args, packet, true);
        }
      }

    }

    //std::cout<<"before close pf"<<std::endl;
    if (pf)
      fclose (pf);
    delete node;
  }
    //recNo = header.segmentNum;
	
  

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

  recv_args.isRunning = false;
  send_args.isRunning = false;
  t1.join();
  t2.join();
  t3.join();
  close(send_socket);
  close(receive_socket);
}