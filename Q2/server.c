
#include "packet.h"

#define PORT 88890
#define WINDOWSIZE 10


char* print_time();


void die(char *s){
    perror(s);
    exit(1); 
}

int main(){

    struct sockaddr_in si_me, si_relay;
    int s, i, slen = sizeof(si_relay) , recv_len;
    char buff[10*PACKET_SIZE];
    memset(buff,'0',sizeof(buff));
    int baseptr = 0;
    int seq = -1;

//create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket");
    }

// zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));

// Filling out the address details for binding the socket    
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT); 
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    // si_me.sin_addr.s_addr = inet_addr("127.0.0.1");

//bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        die("bind");
    }

// Opening file to upload onto
    FILE* fp;

    fp = fopen("tg.txt", "a+");
    if(fp == NULL){
        printf("[-] Error opening file\n");
        return 1;
    }
    printf("[+] File opened/created\n");
    fseek(fp, 0, SEEK_SET);
    fd_set readfds;

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 2;

    struct pkt pkt_upload;

    printf("%-12s%-14s%-16s%-8s%-13s%-10s%-10s\n", "Node Name", "EventType", "Timestamp", "Packet","Seq. No", "Source", "Dest"  );


    while(1){
    fflush(stdout);

// Receiving packet from the relay for ack.
    if (( recv_len = recvfrom(s, &pkt_upload, sizeof(pkt), 0, (struct sockaddr *) &si_relay, &slen)) == -1){
        die("recvfrom()");
    }
    char rel[7] = "RELAY";
    if(pkt_upload.sequence%2 == 0)
        strcat(rel, "2");
    else     
        strcat(rel, "1");

    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "SERVER","R",print_time(), "DATA", pkt_upload.sequence, rel,"SERVER" );
    pkt_upload.data = 0;
    
// Sending the first ack to the server ; recv and send must always come in pairs. There can be no send if there is no recv.
    if (sendto(s, &pkt_upload, sizeof(pkt) , 0 , (struct sockaddr *) &si_relay, sizeof(si_relay))==-1){
        die("sendto()");
    }
    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "SERVER","S",print_time(), "ACK", pkt_upload.sequence, "SERVER",rel );


    if(pkt_upload.sequence < baseptr)           // Duplicate ACK
        continue;

    if(baseptr == pkt_upload.sequence){         // In-order packet

        fwrite(pkt_upload.dat, pkt_upload.size, 1, fp);
        baseptr++;
        
        int forward = 0; 
        if( buff[(baseptr % WINDOWSIZE)*PACKET_SIZE] != '0'){
            for(int inb = baseptr ; inb < baseptr+WINDOWSIZE ; inb ++){

                if( buff[(inb % WINDOWSIZE)*PACKET_SIZE] != '0'){

                    for(int index = 0 ; index < PACKET_SIZE ; index ++){
                        fputc(buff[((inb % WINDOWSIZE)*PACKET_SIZE) + index], fp);
                    }
                    buff[(inb % WINDOWSIZE)*PACKET_SIZE] = '0';
                    forward++;
                }else
                    break;
            }
        }

        baseptr += forward;

    }
    else{               // Out-of-order, we have to store the data in the appropriate position of the buffer
        for(int cpy = 0 ; cpy < pkt_upload.size; cpy ++){
            buff[ ( (pkt_upload.sequence%WINDOWSIZE)*PACKET_SIZE ) + cpy] = pkt_upload.dat[cpy];
        }
        if(pkt_upload.size<PACKET_SIZE)
            buff[( (pkt_upload.sequence%WINDOWSIZE)*PACKET_SIZE ) + pkt_upload.size ] = '\0';
    }

    if(pkt_upload.last_pkt==1){
        seq = pkt_upload.sequence;
    }
    if(baseptr > seq && seq > 0)            // Exit Condition
        break;

    }
    close(s);
    return 0;


}


char* print_time(){
    char* str = (char*) malloc(20);
    int x;
    time_t now;
    struct timeval tv;
    struct tm* ptr;
    now = time(NULL);
    ptr = localtime(&now);
    gettimeofday(&tv, NULL);
    x  = strftime(str, 20, "%H:%M:%S", ptr);

    char arr[8];
    sprintf(arr, ".%06ld", tv.tv_usec);
    strcat(str, arr);
    return str;
   

}
