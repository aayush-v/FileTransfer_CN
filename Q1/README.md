
# Execution Instructions

Since this is a TCP connection, please run the server before running the client.


### Other Details - Methodology 

Timers - Each channel has a dedicated timer for it. For this I have used the time() which gets updated for each channel whenever a new packet is sent from it. To check for timeout, the difference between the present time and the saved sending time is compared with the timeout limit. Also select function is used with a time limit lower than the TIMEOUT time.
This is to break out of the loop and check frequently in case of no read input. 
Both the timers are independent of one another

Multiple connections - Since we require two channels, two sockets are created to establish a TCP connection. For this , both the sockets were saved as an int array which made them faster and easy to index and use simultaneously.

Cases where the file is not a multiple of the PACKET_SIZE are handled.
The size of the buffer is 10 times the PACKET_SIZE. In case more out-of-order packets are received by the server, they are dropped until the correct packet arrives.

The default values for variables mentioned in the assignment statement have been set. 
Everything is working as per the specifications.
