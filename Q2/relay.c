
#include "packet.h"

#define PDR 10
#define PORTTO 88890        // PORT OF THE SERVER
#define EXITTIME 10

void die(char *s){
    perror(s);
    exit(1); 
}

char* print_time();

int main(int argc, char **argv){

    int portnum = atoi(argv[1]);        // On which I will receive from client

    struct sockaddr_in si_me, si_server, si_client, si_accept;
    int s, slen = sizeof(si_client) ;

//create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket");
    }


//Addressing for relay (me)
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portnum); 
    // si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    si_me.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    int me_len = sizeof(si_me);


    printf("Attempting to bind \n");
//bind socket to port for receiving messages
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
        die("bind");
    }
    printf("[+] Binding is successful\n");


//Addressing for server for sending pkt
    memset((char *) &si_me, 0, sizeof(si_me));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORTTO); 
    si_server.sin_addr.s_addr = inet_addr("127.0.0.1");
    int slen1 = sizeof(si_server);

    srand(time(NULL));

    pid_t pid;
    struct pkt pkt1;
    int recv_len;
    int slenacc = sizeof(si_accept);
    int out = -1;
    int r;
    
    printf("%-12s%-14s%-16s%-8s%-13s%-10s%-10s\n", "Node Name", "EventType", "Timestamp", "Packet","Seq. No", "Source", "Dest"  );


// To save the address of the client in si_client for future use
    if (( recv_len = recvfrom(s, &pkt1, sizeof(pkt), 0, (struct sockaddr *) &si_client, &slen)) == -1){
            die("recvfrom()");
        }
    char rel[7] = "RELAY";
    if(pkt1.sequence%2 == 0)
        strcat(rel, "2");
    else     
        strcat(rel, "1");

    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"R",print_time(), "DATA", pkt1.sequence, "CLIENT",rel );
    
// Sending the first ack to the server ; recv and send must always come in pairs. There can be no send if there is no recv.
    if (sendto(s, &pkt1, sizeof(pkt) , 0 , (struct sockaddr *) &si_server, sizeof(si_server))==-1){
        die("sendto()");
    }
    printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"S",print_time(), "DATA", pkt1.sequence, rel,"SERVER" );


    struct timeval tv;    
    fd_set readfds;             // For the select function
    tv.tv_sec = EXITTIME;
    tv.tv_usec = 0;

    //keep listening for data
    while(1){
        r = rand()%100;         // For PDR
        fflush(stdout);

        FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        int activity = select(s+1, &readfds , NULL , NULL , &tv);  
    
        if(activity == 0){
            printf("TIMEOUT\n");
            close(s);
            return 0;
        }

     //try to receive some data from either a client or the server, this is a blocking call
        if (( recv_len = recvfrom(s, &pkt1, sizeof(pkt), 0, (struct sockaddr *) &si_accept, &slenacc)) == -1){
            printf("DEAD recv");
            die("recvfrom()");
        }

        if(pkt1.last_pkt == 1 && pkt1.data == 0)
            out = 1;

        if(recv_len <=0){
            printf("Received empty string");
            continue;
        }

        if( (pid= fork())==0){      // Fork, the chld process will send a packet to either the server or the client while the parent gets ready to receive

            if(pkt1.data ==1){

                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"R",print_time(), "DATA", pkt1.sequence, "CLIENT",rel );
                if((r)<PDR){                // Data pkt for the server
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"D",print_time(), "DATA", pkt1.sequence, "CLIENT",rel );
                    exit(0);}

                // Packet to be sent to the server after some PDR
                int tim = (rand()%10000)/5;     //Delay
                usleep(tim);
                
                // printf("Sent %d to server: -%d\n", pkt1.sequence,r);
                if (sendto(s, &pkt1, sizeof(pkt) , 0 , (struct sockaddr *) &si_server, sizeof(si_server))==-1){
                    die("sendto()");
                }
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"S",print_time(), "DATA", pkt1.sequence,rel, "SERVER" );

            }else if(pkt1.data ==0){        // ACK to be sent to the client
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"R",print_time(), "ACK", pkt1.sequence, "SERVER",rel );

                if (sendto(s, &pkt1, sizeof(pkt) , 0 , (struct sockaddr *) &si_client, sizeof(si_client))==-1){
                    die("sendto()");
                }
                printf("%-15s%-8s%-20s%-10s%-10d%-10s%-10s\n", rel,"S",print_time(), "ACK", pkt1.sequence,rel, "CLIENT" );
            }
            exit(0);
        }     
    }
    close(s);
    return 0;
}


// Relay to server - Work as a normal client, so relay would know the SERVER_PORT of the server before hand. sendto() 
// Server to relay - Server should function normally. As a client, relay know where it is receiving from. recvfrom() 
// Client to relay - Work as a server. The client knows the RELAY_PORT of the relay before hand. recvfrom() 
// Relay to client - Take addr from the accept struct and use that. sendto()


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