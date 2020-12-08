### Execution Instructions ###

While running executable for RELAY, please use 88889 as a command line argument for one really and 888887 as the command line argument for the second relay.

Ex. 	gcc relay.c -o relay
	./relay 88889
This would set up one of the relays.

Kindly run the relays and the server before executing the client.


### Other Details - Methodology ###

Timer - There is one dedicated timer for an entire window. For its implementation, a mix of the select function and the time() function have been used. The select function is used to check at regular intervals which are smaller than the actual TIMEOUT. The time() function is used to check whether or not timeout has occurred. Every time a packet is sent, the time() function sets a new start_time for the window.

Sequence number - Since there has been no mention about sequence numbers in the assignment statement, I have used a unique number for every packet. The minimum required sequence numbers in Selective Repeat Protocol is twice the window size, but here I have create new sequence numbers with every new packet.
We can still change the sequence numbers required into twice of window size by taking the modulo of every sequence number with (2 * WINDOW_SIZE) 

Assumption - As mentioned in the assignment document, the size of the input file should be a multiple of PACKET_SIZE for no extra garbage characters.

On timeout, only the unACKed packets in the window are retransmitted.

Fork() function is used in the relay. The child process sends a message to either the server or client while the parent process is ready to receive new packets simultaneously.
A few packets might be dropped due to the set PDR.

The buffer size in the server is equal to the size of the window. In this protocol, the buffer can never overflow for a buffer of this size. The buffer ensures that the packets are sent to the output file in-order even if they reach the server out of order.

The relay exits on its own if it doesn't receive any packet for 10 seconds. This can be reduced too.
The server and client exit as soon as the file transfer is complete. 

The default values have been set as per the specifications.
Default value of window size is 10, it is used as a macro definition and hence can easily be varied.
Everything is working as per the specifications.

