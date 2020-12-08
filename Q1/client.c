#include "packet.h"

#define TIMEOUT 2

FILE* fileOpen(char* path, char* opentype);
int createTCPSocket();
int createTCPConnection(int socket, struct sockaddr_in *si_other);
struct pkt* fillPacket(struct pkt *pkt1, char buff[], int seq, int channel);



int main(){

    struct sockaddr_in si_other;
    int slen = sizeof(si_other);
    int flag = -1;

    //Creating File pointer
    FILE* fp;
    fp =fileOpen("input.txt", "r");
    if(fp== NULL){ 
        printf("[-] Couldnt open the file\n");
        return 1;
    }else
        printf("[+] %d File opened/created\n",fp);
 

//int sockets for the two channels
    int sockfd[2];

// Creating both sockets
    sockfd[0] = createTCPSocket();
    sockfd[1] = createTCPSocket();
    if(sockfd[0]>0 && sockfd[1]>0)
        printf("[+] Both channels are successfully created\n");
    else
        return 1;
    
// Writing details for struct sockaddr of Server
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(5001); // port 
    si_other.sin_addr.s_addr = inet_addr("127.0.0.1"); 

//Connecting to the sockets....
    if(connect(sockfd[0], (struct sockaddr *)&si_other, slen)<0 || connect(sockfd[1], (struct sockaddr *)&si_other, slen)<0){
        printf("[-] Error: Could not connect to socket, please check server\n");
        return 1;    
    }
    printf("[+] Both sockets connected..\n");

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    time_t chan1_start[2];
    chan1_start[0]= time(NULL);
    chan1_start[1]= time(NULL);
    int tout = TIMEOUT;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd[0], &readfds);
    FD_SET(sockfd[1], &readfds);
    int fd_max;
    if(sockfd[0]>sockfd[1])
        fd_max = sockfd[0];
    else
        fd_max = sockfd[1];

    char buff1[PACKET_SIZE];
    memset(buff1, '\0', sizeof(buff1)); 
    struct pkt pkt1[2];

    while(1){
        for(int to = 0; to<2; to++){
            if((time(NULL) - chan1_start[to])>= tout ){
                write(sockfd[to], &pkt1[to], sizeof(pkt));
                chan1_start[to] = time(NULL);
            }
        }
        if(flag !=-1){
            sleep(0.05);
            break;}

        FD_ZERO(&readfds);
        FD_SET(sockfd[0], &readfds);
        FD_SET(sockfd[1], &readfds);

        int activity = select(fd_max+1, &readfds , NULL , NULL , &tv);  

        if ((activity<0) && (errno!=EINTR)){   
            printf("error\n");   
        }

        else{
            struct pkt pkt2;

            for(int rc = 0 ; rc < 2 ; rc++ ){       // Iterate to find which socket is activated.

                if( FD_ISSET(sockfd[rc], &readfds)){
                    read(sockfd[rc], &pkt2, sizeof(pkt)); 
                    if(pkt2.dat!= "") 
                        printf("RCVD ACK: for PKT with Seq. No. %d from channel %d \n", pkt2.sequence, pkt2.channel);         

                    if(rc == pkt2.channel && flag ==-1){
                        memset(buff1, '\0', sizeof(buff1));
                        int se = ftell(fp);
                        int bf1_len = fread(buff1,PACKET_SIZE,1,fp);
                        fillPacket(&pkt1[rc], buff1, se, rc);
                    
                        char c;
                        if(strlen(buff1) < PACKET_SIZE || feof(fp) || (c = fgetc(fp))==EOF){
                            pkt1[rc].last_pkt = 1;
                            flag = rc;
                        }else
                            pkt1[rc].last_pkt = 0; 
                        fseek(fp,-1, SEEK_CUR);

                        write(sockfd[rc], &pkt1[rc], sizeof(pkt));      // Packet sent
                        chan1_start[rc] = time(NULL);                   // Timer started
                        printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n",  pkt1[rc].sequence, pkt1[rc].size, pkt1[rc].channel);
                        
                        if(flag!=-1){  
                            sleep(tout);
                        }
                    }
                }
            }   
        }
    }
    fclose(fp);

    printf("Returning\n");
    return 0;
}




struct pkt* fillPacket(struct pkt *pkt1, char buff[], int seq, int channel){
    pkt1->sequence = seq;
    pkt1->size = strlen(buff);
    pkt1->channel = channel;
    strcpy((*pkt1).dat, buff);
    pkt1->data = 1;
    if(strlen(buff)<PACKET_SIZE)
        (pkt1->dat)[strlen(buff)] = '\0';

    return pkt1;
}


int createTCPSocket(){
    int s;
    if((s = socket(AF_INET, SOCK_STREAM, 0))<0){
        printf("[-] Error : Could not create socket\n");
        return -1;
    } 
    return s;
}

int createTCPConnection(int socket, struct sockaddr_in *si_other){
    int slen = sizeof(si_other);
    return connect(socket, (struct sockaddr *)&si_other, slen);
}


FILE* fileOpen(char* path, char* opentype){
    FILE* fp;
    fp = fopen(path, opentype);
    if(fp == NULL){
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);

    return fp;
}