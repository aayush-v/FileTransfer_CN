#include "packet.h"

#define WINDOWSIZE 10
#define TIMEOUT 2

int createUDPSocket();
void specifyRelayAddress(struct sockaddr_in pt[2], int p1, int p2);
struct pkt* fillPacket(struct pkt *pkt1, char buff[], int seq);
char* print_time();


void die(char *s){
    perror(s);
    exit(1); 
}



int main(){

    struct sockaddr_in si_other[2], si_accept;      // For the two relays and for receiving
    int slen1 = sizeof(si_other[0]),slen2 = sizeof(si_other[1]), slenacc = sizeof(si_accept);

    FILE* fp;                           //Creating File pointer
    fp = fopen("input.txt", "r");
    if(fp == NULL){
        printf("[-] Error opening file\n");
        return 1;
    }
    printf("File opened/created\n");
    
    struct timeval tv;
    tv.tv_sec = TIMEOUT/2;
    tv.tv_usec = 0;
    time_t chan1_start= time(NULL);
    time_t timeout = TIMEOUT;

// Creating a client socket
    int sockfd = createUDPSocket();
    if(sockfd<0)
        exit(0);
    printf("Socket is %d", sockfd);


    si_other[0].sin_family = AF_INET;
    si_other[0].sin_port = htons(88887); // port on which to send
    si_other[0].sin_addr.s_addr = inet_addr("127.0.0.1"); 
    si_other[1].sin_family = AF_INET;
    si_other[1].sin_port = htons(88889); // port 
    si_other[1].sin_addr.s_addr = inet_addr("127.0.0.1"); 

    fd_set readfds;             // For the select function
    char buff1[20];             // Storing the data read from the input file
    int windowsize = WINDOWSIZE, recv_len, lastseq = -1, baseptr = 0, last_sent = 0;
    
    int ackedpkt[windowsize];   // save the sequence number of the acked pkt in this
    for(int x = 0; x<windowsize; x++)
        ackedpkt[x] = -1;

    struct pkt pkt_send[windowsize], pkt_recv;
    int flag_to = 0;

    printf("Beginning the transmission..\n");
    printf("%-12s%-14s%-16s%-8s%-13s%-10s%-10s\n", "Node Name", "EventType", "Timestamp", "Packet","Seq. No", "Source", "Dest"  );
    while(1){

        for(int iter = last_sent; (iter< baseptr + windowsize) && lastseq < 0; iter ++, last_sent++){
            memset(buff1, '\0', sizeof(buff1));
            int index = iter%windowsize;

            int seq = ftell(fp)/PACKET_SIZE;
            int bf1_len = fread(buff1,PACKET_SIZE,1,fp);
            
            fillPacket(&pkt_send[index], buff1, seq );
            char c;
            if(strlen(buff1) < PACKET_SIZE || feof(fp) || (c = fgetc(fp))==EOF){
                pkt_send[index].last_pkt = 1;
                lastseq = iter;
            }else
                pkt_send[index].last_pkt = 0;        
            fseek(fp,-1, SEEK_CUR);
            chan1_start = time(NULL);


            if((pkt_send)[index].sequence%2 == 0){           //Send pkt to R2
                if (sendto(sockfd, &pkt_send[index], sizeof(pkt) , 0 , (struct sockaddr *) &si_other[0], sizeof(si_other[0]))==-1){
                    die("sendto()");
                }
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","S",print_time(), "DATA", pkt_send[index].sequence, "CLIENT","RELAY2" );
            }else{                                           // Send pkt to R1
                if (sendto(sockfd, &pkt_send[index], sizeof(pkt) , 0 , (struct sockaddr *) &si_other[1], sizeof(si_other[1]))==-1){
                    die("sendto()");
                }
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","S",print_time(), "DATA", pkt_send[index].sequence, "CLIENT","RELAY1" );
            }
            if(lastseq>=0){
                break;
            }
        }

       
        while(1){           // INNER WHILE

            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            int activity = select(sockfd+1, &readfds , NULL , NULL , &tv);  

            if ((activity<0) && (errno!=EINTR)){    // Select function error
                printf("select error");   
            }

            else if(activity == 0){              // Checks for timeout possibility
                if(time(NULL) - chan1_start >= timeout){
                    flag_to = 1;
                    break;                      // Timeout has happened, move out of inner while
                }else
                    continue;                   // No timeout yet, lets wait for another pkt until we touch timeout.
            }
            else{                               // Received a packet 
                if((recv_len = recvfrom(sockfd, &pkt_recv, sizeof(pkt), 0, (struct sockaddr *) &si_accept, &slenacc)) == -1){
                    die("recvfrom()");
                }

                char rel[7] = "RELAY";
                if(pkt_recv.sequence%2 == 0)
                    strcat(rel, "2");
                else     
                    strcat(rel, "1");
                
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","R",print_time(), "ACK", pkt_recv.sequence, rel,"CLIENT" );
                int acked = pkt_recv.sequence;

                if(acked < baseptr || acked > (baseptr+windowsize)){        // If ack if lesser than basepointer, old ack
                    continue;}
                
                int i = (acked%windowsize);
                ackedpkt[i] = acked;           // Packet is being marked received by adding it to the acked array

                if(acked == baseptr){    
                    while(ackedpkt[i] == baseptr){      // Can write this in a function, this is moving the baseptr by one until
                        baseptr+=1;                     // the baseptr doesnt equal the entry in the acked array i.e. basically 
                        i++;                            // a packet that has not been acked yet. So THAT becomes the new baseptr.
                        i = i % windowsize;
                    }
                    if(baseptr > lastseq && lastseq>0){
                        printf("All received, exiting...\n");
                        exit(0);
                    }
                    break;                     
                }
            }
        }
        if(baseptr > lastseq && lastseq>0){
            printf("All received, exiting...\n");
            exit(0);
        }    
    
// Now we should handle timeout case
    if(flag_to==1){
        for(int resend = baseptr; resend<baseptr+windowsize; resend ++){
            
            if(resend>lastseq && lastseq>0){
                // printf("End broken\n");
                break;}

            int iter = resend%windowsize;
            if(ackedpkt[iter] != resend){

                if(pkt_send[iter].sequence%2 ==0){
                    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","TO",print_time(), "DATA", pkt_send[iter].sequence, "CLIENT","RELAY2" );                
                    if (sendto(sockfd, &pkt_send[iter], sizeof(pkt) , 0 , (struct sockaddr *) &si_other[0], sizeof(si_other[0]))==-1){
                        die("sendto()");
                    }
                    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","RE",print_time(), "DATA", pkt_send[iter].sequence, "CLIENT","RELAY2" );                
                }
                else{
                    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","TO",print_time(), "DATA", pkt_send[iter].sequence, "CLIENT","RELAY1" );
                    if (sendto(sockfd, &pkt_send[iter], sizeof(pkt) , 0 , (struct sockaddr *) &si_other[1], sizeof(si_other[1]))==-1){
                        die("sendto()");
                    }
                    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", "CLIENT","RE",print_time(), "DATA", pkt_send[iter].sequence, "CLIENT","RELAY1" );
                }
            }
        }
        chan1_start = time(NULL);
        flag_to = 0;  
        } 
    }
    close(sockfd);
    return 0;

}



int createUDPSocket(){
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0 ){
        printf("[-] Error : Could not create socket\n");
        return -1 ;
    }
    printf("[+] Client socket is created..\n");
    return sockfd;
}

struct pkt* fillPacket(struct pkt *pkt1, char buff[], int seq){
    pkt1->sequence = seq;
    pkt1->size = strlen(buff);
    strcpy((*pkt1).dat, buff);
    pkt1->data = 1;
    if(strlen(buff)<PACKET_SIZE)
        (pkt1->dat)[strlen(buff)] = '\0';

    return pkt1;
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

