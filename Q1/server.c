#include "packet.h"

#define PORT 5001 
#define PDR 10


int main(int argc , char *argv[])   
{   
    int master_socket , add_len , new_socket , client_socket[2] , activity, i  , sd;   
    int max_sd;   
    struct sockaddr_in address;   
    struct pkt pkt1;
    char orderbuffer[(10 * PACKET_SIZE)+1];
    memset(orderbuffer, '\0', sizeof(orderbuffer));
    
    //set of socket descriptors  
    fd_set readfds;   
         
    srand(time(NULL));
    FILE* fp;
    fp = fopen("targ.txt", "a+");
    if(fp==NULL)
        printf("[-] File could not be opened\n");
    else
        printf("[+] File opened successfully\n");

    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < 2; i++)   
        client_socket[i] = 0;   
    //create a master socket  
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0){   
        perror("socket failed");   
        exit(EXIT_FAILURE);   
    }   
     
    //type of socket created  
    address.sin_family = AF_INET;   
    address.sin_addr.s_addr = INADDR_ANY;   
    address.sin_port = htons( PORT );   
         

    //BIND the socket to localhost port 8888  
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0){   
        perror("bind failed");   
        exit(EXIT_FAILURE);   
    }
    printf("[+] Binding successful");

    //try to specify maximum of 2 pending connections for the master socket  
    if (listen(master_socket, 2) < 0){   
        perror("listen");   
        exit(EXIT_FAILURE);   
    }   
         
    //accept the incoming connection  
    add_len = sizeof(address);   
    printf("Waiting for connections ...");   

    int end_fp=0, exit_state=0, exit_while;     // Variables to set the exit condition
    int file_offset = 0;

    while(1){       
        FD_ZERO(&readfds);   //clear the socket set      
        FD_SET(master_socket, &readfds);   //add master socket to set
        max_sd = master_socket;   
             
        //add child sockets to set  
        for ( i = 0 ; i < 2 ; i++){   
            sd = client_socket[i];    //socket descriptor 
              
            if(sd > 0)   //if valid socket descriptor then add to read list
                FD_SET(sd,&readfds);     
            if(sd > max_sd)    //highest file descriptor number, need it in the select function
                max_sd = sd;   
        }   

        activity = select(max_sd + 1, &readfds , NULL , NULL , NULL);   
        if ((activity<0) && (errno!=EINTR)){   
            printf("error\n");   
        }   
             
        if (FD_ISSET(master_socket, &readfds)){   // Connection attempt from client has been done
            if ((new_socket = accept(master_socket,(struct sockaddr *)&address, (socklen_t*)&add_len))<0){   
                perror("accept");   
                exit(EXIT_FAILURE);   
            }

            puts("Connection created");    
            for (i = 0; i < 2; i++){   
                if( client_socket[i] == 0){   // indicates empty position   
                    client_socket[i] = new_socket;
                    pkt1.channel = i;   
                    write(new_socket, &pkt1, sizeof(pkt));   
                    break;   
                }
            }   
        }


        for(int rc = 0 ; rc < 2 ; rc++ ){
            if(FD_ISSET(client_socket[rc], &readfds)){
                        
                int randomnumber = (rand() % 100);
                pkt dump_pkt;
                if(randomnumber < PDR){             // Drop packets according to PDR set
                    read(client_socket[rc], &dump_pkt, sizeof(pkt));
                    printf("RCVD PKT: Seq. no. %d of size %d Bytes from channel %d\n", dump_pkt.sequence, dump_pkt.size, dump_pkt.channel);
                    fflush(stdout);
                    continue;
                }      

        // If here, then packet not dropped due to PDR
                read(client_socket[rc], &pkt1, sizeof(pkt));
                printf("RCVD PKT: Seq. no. %d of size %d Bytes from channel %d\n", dump_pkt.sequence, dump_pkt.size, dump_pkt.channel);
                if(pkt1.sequence != file_offset){   //Drop packets here if buffer is full
                    if((strlen(orderbuffer) == sizeof(orderbuffer)) || (strlen(orderbuffer) + 1 == sizeof(orderbuffer)) ){
                        fflush(stdout);
                        continue;
                    }
                }
                pkt1.data = 0;
                write(client_socket[rc], &pkt1, sizeof(pkt));
                printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", pkt1.sequence, pkt1.channel);

                if(pkt1.last_pkt == 1){
                    //(EXIT STAAATE HAS BEEN MADE ONE!)
                    exit_state = 1;
                    end_fp = pkt1.sequence + pkt1.size;
                }

                if(pkt1.sequence == file_offset){
                    
                    fwrite(pkt1.dat,pkt1.size,1,fp);        // Write the packet data in the file
                    fwrite(orderbuffer,strlen(orderbuffer),1,fp);                // Write the buffer data in the file
                    file_offset = file_offset + sizeof(pkt1.dat) + strlen(orderbuffer);
                    memset(orderbuffer, '\0', sizeof(orderbuffer));
                    
                }else if(pkt1.sequence > file_offset)
                        strcat(orderbuffer,pkt1.dat);               
                
                if(exit_state == 1 && file_offset>= end_fp){
                    exit_while = 1;
                    break;
                }
            }      
        }
                if( exit_while == 1){
                    break;
                }
    }
    close(master_socket);
    printf("Closing server....\n");
    fclose(fp);
    return 0;
} 
