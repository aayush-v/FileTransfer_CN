#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h>  
#include <sys/types.h>  
#include <netinet/in.h>  
#include <sys/time.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdbool.h>

#define PACKET_SIZE 100


typedef struct pkt{
        int size;
        int sequence;
        bool last_pkt;
        bool data;  //1 means Data pkt, 0 means Ack pkt
        char dat[PACKET_SIZE];
    }pkt;
