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
  if (argc != 2) {
    perror("arguments error");
    return 1;
  }
  int port = std::stoi(argv[1]);
  std::cout<<"prot is"<<port<<std::endl;
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;
  char incoming_message[PACKET_LENGTH];
  pthread_t tid[2];
  std::map<count, std::string>::iterator filename_it;
  std::map<count, long>::iterator filesize_it;
  std::map<count, count>::iterator nextsegment_it;
  std::map<count, count>::iterator sendbase_it;
  bool finished = false, timewait = false;
  bool finAckRecved = false;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  count deletedId;

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
  server.sin_port = htons(port);

  //checks connection
  if (bind(receive_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection error");
    return 1;
  }
  printf("Main\n");
  puts("Bind");

  //server.sin_addr.s_addr = htonl(INADDR_ANY);
  //server.sin_family = AF_INET;
  //server.sin_port = htons(CLIENTPORT);

  //if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
  //  perror("Connection error");
  //  return 1;
  //}
  //puts("Connected");

  sendArgs send_args;
  recvArgs recv_args;
  send_args.socket = send_socket;
  send_args.isRunning = true;
  send_args.isServer = true;
  recv_args.socket = receive_socket;
  recv_args.clientIp = "";
  recv_args.isRunning = true;
  recv_args.isServer = true;

  std::thread t1(send_data, &send_args);
  std::thread t2(receive_data, &recv_args);
  std::thread t3(check_timeout, &send_args);

  while (true) {
  	Node* node = NULL;
    recv_args.lock.lock();
	  if (!recv_args.queue.empty()) {
      node = recv_args.queue.front();
		  recv_args.queue.pop();
	  }
    recv_args.lock.unlock();

	  if (!node) {
      if (timewait) {
        end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        if (elapsed_seconds.count() > 1.0) {
          id_nextsegment.erase (deletedId);
          id_filename.erase (deletedId);
          id_sendbase.erase (deletedId);
          id_filesize.erase (deletedId);
          id_offset.erase (deletedId);
          deletedId = 0;
          timewait = false;
        }
      }
	    continue;
    }

    FILE *pf = NULL;
    Header* header = node->packet->header;
    checkBuffer(&send_args, node);

    if (header->isSyn) {
      sendbase_it = id_sendbase.find(header->initID);
      if (sendbase_it == id_sendbase.end()) { 
        recv_args.lock.lock();
          inet_aton(recv_args.clientIp.c_str(), &server.sin_addr);
        recv_args.lock.unlock();
        //server.sin_addr.s_addr = htonl(INADDR_ANY);
        server.sin_family = AF_INET;
        server.sin_port = htons(CLIENTPORT);
        if (connect(send_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
          perror("Connection error");
          return 1;
        }
        puts("Connected");

        count seq = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
        Packet* packet = new Packet(header->initID, seq, 
                                    (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                    header->window, false, true, false, true, false);
        sendPacket(&send_args, packet, true);
        id_sendbase[header->initID] = 0;
        finished = false;
        timewait = false;
        finAckRecved = false;
      }
      //std::cout<<"finished syn"<<std::endl;
    }
    else if (header->isFin) {
      //std::cout<<"isFin"<<std::endl;
      sendbase_it = id_sendbase.find(header->initID);
      if (sendbase_it == id_sendbase.end()) {
        delete node;
        continue;
      }
      if (header->isAck) {
        finAckRecved = true;
        delete node;
        continue;
      }

      if (!finAckRecved) {
        delete node;
        continue;
      }

      Packet* packet1 = new Packet(header->initID, header->ackNum,
                                   (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                   header->window, false, true, false, false, true);
      sendPacket(&send_args, packet1, false);

      timewait = true;
      deletedId = header->initID;
      start = std::chrono::system_clock::now();
    }

    else if (header->isAck) {
      sendbase_it = id_sendbase.find(header->initID);
      if (sendbase_it == id_sendbase.end()) {
        delete node;
        continue;
      }
      //std::cout<<"isAck"<<std::endl;
      filename_it = id_filename.find(header->initID);
      window = header->window;
      if (filename_it != id_filename.end())
        pf = fopen(filename_it->second.c_str(), "r");
      else {
        char filename[100];
        strncpy(filename, node->packet->message+HEADER_SIZE, header->dataLength);
        filename[header->dataLength] = '\0';
        printf("length:%d, filename:%s\n", header->dataLength, filename);
        pf=fopen(filename, "r");
        long fileSize = 0;
        if (pf) {
          fseek (pf , 0 , SEEK_END);
          fileSize = ftell (pf);
          rewind (pf);
        }
        id_filename[header->initID] = std::string(filename);
        id_filesize[header->initID] = fileSize;
        id_sendbase[header->initID] = header->ackNum;
        id_nextsegment[header->initID] = header->ackNum;
        id_offset[header->initID] = 0;
      }

      //std::cout<<"after filename"<<std::endl;
      if (pf==NULL) {
        finished = true;
        fputs ("file error",stderr);
      }
      else {
        if (lessthan(header->ackNum, id_sendbase[header->initID])) {
          fputs ("error occured",stderr);
          delete node;
          continue;
        }

        auto it = buffer.begin();
        while (it != buffer.end() && (*it)->ackNum != header->ackNum) {
          it++;
        }
        if (it != buffer.end() && (*it)->isAcked == true) {
          //fputs ("duplicated ack recevied",stderr);
          delete node;
          continue;
        }
        if (it != buffer.end())
          (*it)->isAcked = true;
        it = buffer.begin();
        while (it != buffer.end() && (*it)->isAcked == true) {
          id_sendbase[header->initID] = (*it)->ackNum;
          //std::cout<<"update sendbase to "<<id_sendbase[header->initID]<<std::endl;
          delete *it;
          buffer.erase(it);
          it = buffer.begin();
        }

        while (((id_nextsegment[header->initID] - id_sendbase[header->initID] + MAX_SEGMENT_NUMBER) % MAX_SEGMENT_NUMBER) < window)
        {
          Packet* packet = new Packet(header->initID, id_nextsegment[header->initID],
                                      (header->segmentNum + header->dataLength) % MAX_SEGMENT_NUMBER,
                                      header->window);

          size_t length = fileRead(pf, id_offset[header->initID], packet->message);
          //std::cout<<"------"<<id_nextsegment[header->initID]<<", "<<id_offset[header->initID]<<", "<< id_sendbase[header->initID] << ", " <<
          //(id_nextsegment[header->initID] - id_offset[header->initID] + MAX_SEGMENT_NUMBER) % MAX_SEGMENT_NUMBER<<", "<<
          //(id_nextsegment[header->initID] + length - id_sendbase[header->initID] + MAX_SEGMENT_NUMBER) % MAX_SEGMENT_NUMBER<<std::endl;

          if ((id_nextsegment[header->initID] + length - id_sendbase[header->initID] + MAX_SEGMENT_NUMBER) %
              MAX_SEGMENT_NUMBER > window || length == 0) {
            if (length == 0) {
              //std::cout<<"------------------finished"<<std::endl;
              finished = true;
            }
            delete packet;
            break;
          }
          else{
            packet->header->dataLength = length;
            BufferNode* buffernode = new BufferNode();
            buffernode->ackNum = (id_nextsegment[header->initID] + length) % MAX_SEGMENT_NUMBER;
            buffernode->isAcked = false;
            buffer.push_back(buffernode);
            id_nextsegment[header->initID] = (id_nextsegment[header->initID] + length ) % MAX_SEGMENT_NUMBER;
            id_offset[header->initID] += length;
            sendPacket(&send_args, packet, true);
          }
        }
      }

    }

    if (finished && buffer.empty()) {
      finished = false;
      //std::cout<<"==================received all "<<id_nextsegment[header->initID] + 1 % MAX_SEGMENT_NUMBER<<std::endl;
      Packet* packet = new Packet(header->initID, (id_nextsegment[header->initID] + 1) % MAX_SEGMENT_NUMBER,
                                  (header->segmentNum + header->dataLength + 1) % MAX_SEGMENT_NUMBER,
                                  header->window, false, false, false, false, true);
      sendPacket(&send_args, packet, true);
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