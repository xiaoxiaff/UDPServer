/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include "protocol.h"
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

typedef struct t_args {
	FILE* pf;
	int send_socket;
	int receive_socket;
	int seqNo;
	int recNo;
	long fileSize;
	Node* head;
	pthread_mutex_t lock;
	bool started;
} t_args;

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

void* send_data(void* t_args) {
	struct t_args* args = t_args;
   	char message[PACKET_LENGTH];
	long cur = 0;
	Header header;
	
  	setHeader(&header, args->seqNo, ++args->recNo, 0, true, false, true, false, 0);
  	generatePacket(message, &header, NULL, 0);

    printLog(true, &header);
    if(send(args->send_socket, message, HEADER_SIZE, 0) <0) {        
      printf("Send failed\n");
      exit(1);
  	}
	
  	while (cur < args->fileSize) {
      //printf("received seq:%u, cur:%ld, total:%ld", header.segmentNum, cur, fileSize);
      // if ((args->header)->ackNum == args->seqNo + 1) {
        size_t length = fileRead(args->pf, cur, message);
        //printLog(message, HEADER_SIZE+length);
        cur += length;
	  	setHeader(&header, ++args->seqNo, ++args->recNo, 0, true, false, false, false, 0);
	  	generatePacket(message, &header, message+HEADER_SIZE, length);
      // } else {
      //	continue;
      // }
      if(send(args->send_socket, message, HEADER_SIZE + header.dataLength, 0) <0) {
        perror("Send failed\n");
        exit(1);
      }

      printLog(true, &header);
  	}
	pthread_exit(0);
}

void* receive_data(void* t_args) {
	struct t_args* args = t_args;
    char incoming_message[PACKET_LENGTH];	
	long cur = 0;
	Header header;
		
	int max_num = (args->fileSize)/(PACKET_LENGTH-HEADER_SIZE) + 1;

  	while (cur < max_num) {
	  cur++;
  	  if ( recv(args->receive_socket, incoming_message, sizeof(incoming_message), 0) < 0) {
        puts("Received failed");
        exit(1);
      }

      parseRequest(incoming_message, &header);
      printLog(false, &header);
	  
	}
	
	args->started = false;
	pthread_exit(0);

}

void* check_timeout(void* t_args) {
	struct t_args* args = t_args;

	while (true) {
		if (args->started == false)
			break;
		pthread_yield();
		pthread_mutex_lock(&(args->lock));
			if ((args->head) != NULL) {
				if (((args->head)->packet)->timestamp < (gettime() - 500)) {
					if(send(args->send_socket, ((args->head)->packet)->message, ((args->head)->packet)->length, 0) <0) {
						perror("Send failed\n");
						exit(1);
					}
					((args->head)->packet)->timestamp = gettime();
					args->head = (args->head)->next;
				}
			}
		pthread_mutex_unlock(&(args->lock));
	}
	
	pthread_exit(0);
}


int main(int argc, char *argv[])
{
  //initialize socket and structure
  int send_socket, receive_socket;
  struct sockaddr_in server;
  Header header;
  char incoming_message[PACKET_LENGTH];
  pthread_t tid[2];

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
  


  while (true) {
    count recNo = 0, seqNo = rand() / (RAND_MAX / (MAX_SEGMENT_NUMBER) + 1);
    if( recv(receive_socket, incoming_message, sizeof(incoming_message), 0) < 0) {      
      puts("Received failed");
      return 1;
    }
	
    FILE *pf;
    parseRequest(incoming_message, &header);
    printLog(false, &header);
  
    if (!header.isSyn)
		continue;

  	printf("Syn packet\n");
    char filename[100];
    strncpy(filename, incoming_message+HEADER_SIZE, header.dataLength);
    filename[header.dataLength] = '\0';
    printf("before file ");
    printf("%s, %u\n",filename, header.dataLength);
    pf=fopen(filename, "r");
    if (pf==NULL) {
    	fputs ("File error",stderr);
	    continue;
    }
    recNo = header.segmentNum;

    fseek (pf , 0 , SEEK_END);
    long fileSize = ftell (pf);
    rewind (pf);
	
	t_args t_args;
	
	t_args.pf = pf;
	t_args.send_socket = send_socket;
	t_args.receive_socket = receive_socket;
	t_args.seqNo = seqNo;
	t_args.recNo = recNo;
	t_args.fileSize = fileSize;
	t_args.head = NULL;
	t_args.started = true;
	pthread_mutex_init( &(t_args.lock), NULL);
	
    pthread_create(&(tid[0]), NULL, send_data, &t_args);
    pthread_create(&(tid[1]), NULL, receive_data, &t_args);
    pthread_create(&(tid[2]), NULL, check_timeout, &t_args);
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);
	
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