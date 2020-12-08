#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define PACKET_SIZE 100

typedef struct pkt{
        int size;
        int sequence;
        bool last_pkt;
        bool data;  //1 means Data pkt, 0 means Ack pkt
        bool channel;
        char dat[PACKET_SIZE];
    }pkt;
